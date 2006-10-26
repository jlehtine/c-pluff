/*-------------------------------------------------------------------------
 * C-Pluff, a plug-in framework for C
 * Copyright 2006 Johannes Lehtinen
 *-----------------------------------------------------------------------*/

// Core C-Pluff functions 

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>
#include "../kazlib/hash.h"
#include "cpluff.h"
#include "defines.h"
#include "util.h"
#ifdef CP_THREADS
#include "thread.h"
#endif
#include "internal.h"


/* ------------------------------------------------------------------------
 * Data types
 * ----------------------------------------------------------------------*/

typedef struct dynamic_resource_t dynamic_resource_t;

/// Contains information about a dynamically allocated resource
struct dynamic_resource_t {

	/// Pointer to the resource
	void *resource;	
	
	/// Usage count for the resource
	int usage_count;
	
	/// Deallocation function
	cpi_dealloc_func_t dealloc_func;
};


/* ------------------------------------------------------------------------
 * Variables
 * ----------------------------------------------------------------------*/

/// Implementation information 
static cp_implementation_info_t implementation_info = {
	CP_RELEASE_VERSION,
	CP_API_VERSION,
	CP_API_REVISION,
	CP_API_AGE,
	CP_HOST,
	CP_THREADS
};

/// Number of initializations 
static int initialized = 0;

#ifdef CP_THREADS

/// Framework mutex
static cpi_mutex_t *framework_mutex = NULL;

#elif !defined(NDEBUG)

/// Framework locking count
static int framework_locked = 0;

#endif

/// Fatal error handler, or NULL for default 
static cp_error_handler_t fatal_error_handler = NULL;

/// User data pointer for fatal error handler
static void *fatal_eh_user_data = NULL;

/// Map of in-use dynamic resources
static hash_t *dynamic_resources = NULL;


/* ------------------------------------------------------------------------
 * Function definitions
 * ----------------------------------------------------------------------*/

cp_implementation_info_t * CP_API cp_get_implementation_info(void) {
	return &implementation_info;
}

void CP_LOCAL cpi_lock_framework(void) {
	#if defined(CP_THREADS)
	cpi_lock_mutex(framework_mutex);
#elif !defined(NDEBUG)
	framework_locked++;
#endif
}

void CP_LOCAL cpi_unlock_framework(void) {
#if defined(CP_THREADS)
	cpi_unlock_mutex(framework_mutex);
#elif !defined(NDEBUG)
	assert(framework_locked > 0);
	framework_locked--;
#endif
}

static void reset(void) {
	if (dynamic_resources != NULL) {
		hscan_t scan;
		hnode_t *node;
		
		hash_scan_begin(&scan, dynamic_resources);
		while ((node = hash_scan_next(&scan)) != NULL) {
			dynamic_resource_t *dr = hnode_get(node);
			
			hash_scan_delfree(dynamic_resources, node);
			dr->dealloc_func(dr->resource);
			free(dr);
		}
		hash_destroy(dynamic_resources);
		dynamic_resources = NULL;
	}
#ifdef CP_THREADS
	if (framework_mutex != NULL) {
		cpi_destroy_mutex(framework_mutex);
		framework_mutex = NULL;
	}
#endif
}

int CP_API cp_init(void) {
	int status = CP_OK;
	
	// Initialize if necessary
	do {
		if (!initialized) {
			if (bindtextdomain(PACKAGE, CP_DATADIR CP_FNAMESEP_STR "locale") == NULL) {
				status = CP_ERR_RESOURCE;
				break;
			}
#ifdef CP_THREADS
			if ((framework_mutex = cpi_create_mutex()) == NULL) {
				status = CP_ERR_RESOURCE;
				break;
			}
#endif
			if ((dynamic_resources = hash_create(HASHCOUNT_T_MAX, cpi_comp_ptr, cpi_hashfunc_ptr)) == NULL) {
				status = CP_ERR_RESOURCE;
				break;
			}
		}
		initialized++;
	} while (0);
	
	// Rollback on failure
	if (status != CP_OK) {
		reset();
	}
	
	return status;
}

void CP_API cp_destroy(void) {
	assert(initialized > 0);
	initialized--;
	if (!initialized) {
#ifdef CP_THREADS
		assert(framework_mutex == NULL || !cpi_is_mutex_locked(framework_mutex));
#else
		assert(!framework_locked);
#endif
		cpi_destroy_all_contexts();
		reset();
	}
}

void CP_API cp_set_fatal_error_handler(cp_error_handler_t error_handler, void *user_data) {
	cpi_lock_framework();
	fatal_error_handler = error_handler;
	fatal_eh_user_data = user_data;
	cpi_unlock_framework();
}

void CP_LOCAL cpi_fatalf(const char *msg, ...) {
	va_list params;
	char fmsg[256];
		
	// Format message 
	assert(msg != NULL);
	va_start(params, msg);
	vsnprintf(fmsg, sizeof(fmsg), msg, params);
	va_end(params);
	fmsg[sizeof(fmsg)/sizeof(char) - 1] = '\0';

	// Call error handler or print the error message
	cpi_lock_framework();
	if (fatal_error_handler != NULL) {
		fatal_error_handler(NULL, fmsg, fatal_eh_user_data);
	} else {
		fprintf(stderr, _(PACKAGE_NAME ": FATAL ERROR: %s\n"), fmsg);
	}
	cpi_unlock_framework();
	
	// Abort if still alive 
	abort();
}

int CP_LOCAL cpi_register_resource(void *res, cpi_dealloc_func_t df) {
	int status = CP_OK;
	dynamic_resource_t *dr = NULL;
	
	do {
		if ((dr = malloc(sizeof(dynamic_resource_t))) == NULL) {
			status = CP_ERR_RESOURCE;
			break;
		}
		dr->resource = res;
		dr->usage_count = 1;
		dr->dealloc_func = df;
		cpi_lock_framework();
		if (!hash_alloc_insert(dynamic_resources, res, dr)) {
			status = CP_ERR_RESOURCE;
		}
		cpi_unlock_framework();
		
	} while (0);
	
	// Release resources on failure
	if (status != CP_OK) {
		if (dr != NULL) {
			free(dr);
		}
	}
	
	return status;
}

void CP_LOCAL cpi_use_resource(void *res) {
	hnode_t *node;
	dynamic_resource_t *dr;
	
	cpi_lock_framework();
	node = hash_lookup(dynamic_resources, res);
	if (node == NULL) {
		cpi_fatalf(_("Trying to increase usage count on unknown resource %p."), res);
	}
	dr = hnode_get(node);
	dr->usage_count++;
	cpi_unlock_framework();
}

void CP_API cp_release_info(void *info) {
	hnode_t *node;
	dynamic_resource_t *dr;
	
	assert(info != NULL);
	cpi_lock_framework();
	node = hash_lookup(dynamic_resources, info);
	if (node != NULL) {
		dr = hnode_get(node);
		assert(dr != NULL && info == dr->resource);
		if (--dr->usage_count == 0) {
			hash_delete_free(dynamic_resources, node);
			dr->dealloc_func(info);
			free(dr);
		}
	} else {
		fprintf(stderr, _("ERROR: Trying to release unknown resource %p in cp_release_info.\n"), info);
	}
	cpi_unlock_framework();
}
