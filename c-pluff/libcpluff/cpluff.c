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
 * Constants
 * ----------------------------------------------------------------------*/

/// Logging limit for no logging
#define CP_LOG_NONE 1000


/* ------------------------------------------------------------------------
 * Data types
 * ----------------------------------------------------------------------*/

typedef struct logger_t logger_t;
typedef struct dynamic_resource_t dynamic_resource_t;

/// Contains information about installed loggers
struct logger_t {
	
	/// Pointer to logger
	cp_logger_t logger;
	
	/// User data pointer
	void *user_data;
	
	/// Minimum severity
	cp_log_severity_t min_severity;
	
	/// Context criteria
	cp_context_t *ctx_rule;
};

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

/// Loggers
static list_t *loggers = NULL;

/// Global minimum severity for logging
static int log_min_severity = CP_LOG_NONE;

/// Fatal error handler, or NULL for default 
static cp_fatal_error_handler_t fatal_error_handler = NULL;

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
	if (loggers != NULL) {
		lnode_t *node;
		
		while ((node = list_first(loggers)) != NULL) {
			logger_t *l = lnode_get(node);
			list_delete(loggers, node);
			lnode_destroy(node);
			free(l);
		}
		list_destroy(loggers);
		loggers = NULL;
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
			if ((loggers = list_create(LISTCOUNT_T_MAX)) == NULL) {
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
		cpi_info(NULL, _("The plug-in framework is being shut down."));
		cpi_destroy_all_contexts();
		reset();
	}
}

/**
 * Updates the global logging limits.
 */
static void update_logging_limits(void) {
	lnode_t *node;
	
	log_min_severity = CP_LOG_NONE;
	node = list_first(loggers);
	while (node != NULL) {
		logger_t *lh = lnode_get(node);
		if (lh->min_severity < log_min_severity) {
			log_min_severity = lh->min_severity;
		}
		node = list_next(loggers, node);
	}
}

static int comp_logger(const void *p1, const void *p2) {
	const logger_t *l1 = p1;
	const logger_t *l2 = p2;
	return l1->logger != l2->logger;
}

int CP_API cp_add_logger(cp_logger_t logger, void *user_data, cp_log_severity_t min_severity, cp_context_t *ctx_rule) {
	logger_t l;
	logger_t *lh;
	lnode_t *node;

	assert(logger != NULL);
	
	// Check if logger already exists and allocate new holder if necessary
	l.logger = logger;
	cpi_lock_framework();
	if ((node = list_find(loggers, &l, comp_logger)) == NULL) {
		lh = malloc(sizeof(logger_t));
		node = lnode_create(lh);
		if (lh == NULL || node == NULL) {
			cpi_unlock_framework();
			if (lh != NULL) {
				free(lh);
			}
			if (node != NULL) {
				lnode_destroy(node);
			}
			cpi_error(NULL, _("Logger could not be registered due to insufficient memory."));
			return CP_ERR_RESOURCE;
		}
		lh->logger = logger;
		lh->user_data = user_data;
		list_append(loggers, node);
	} else {
		lh = lnode_get(node);
	}
		
	// Initialize or update the logger holder
	lh->min_severity = min_severity;
	lh->ctx_rule = ctx_rule;
		
	// Update global limits
	update_logging_limits();
	cpi_unlock_framework();

	cpi_debugf(NULL, _("Logger %p was added or updated with minimum severity %d."), logger, min_severity);
	return CP_OK;
}

void CP_API cp_remove_logger(cp_logger_t logger) {
	logger_t l;
	lnode_t *node;
	
	l.logger = logger;
	cpi_lock_framework();
	if ((node = list_find(loggers, &l, comp_logger)) != NULL) {
		logger_t *lh = lnode_get(node);
		list_delete(loggers, node);
		lnode_destroy(node);
		free(lh);
		update_logging_limits();
	}
	cpi_unlock_framework();
	cpi_debugf(NULL, _("Logger %p was removed."), logger);
}

static void log(cp_context_t *ctx, cp_log_severity_t severity, const char *msg) {
	lnode_t *node = list_first(loggers);
	while (node != NULL) {
		logger_t *lh = lnode_get(node);
		if (severity >= lh->min_severity
			&& (lh->ctx_rule == NULL || ctx == lh->ctx_rule)) {
			lh->logger(severity, msg, ctx, lh->user_data);
		}
		node = list_next(loggers, node);
	}
}

void CP_LOCAL cpi_log(cp_context_t *ctx, cp_log_severity_t severity, const char *msg) {
	if (severity >= log_min_severity) {
		log(ctx, severity, msg);
	}
}

void CP_LOCAL cpi_logf(cp_context_t *ctx, cp_log_severity_t severity, const char *msg, ...) {
	if (severity >= log_min_severity) {
		char buffer[256];
		va_list va;
		
		va_start(va, msg);
		vsnprintf(buffer, sizeof(buffer), msg, va);
		va_end(va);
		buffer[sizeof(buffer)/sizeof(char) - 1] = '\0';
		log(ctx, severity, buffer);
	}
}

int CP_LOCAL cpi_is_logged(cp_log_severity_t severity) {
	return severity >= log_min_severity;
}

void CP_API cp_set_fatal_error_handler(cp_fatal_error_handler_t error_handler) {
	fatal_error_handler = error_handler;
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
	if (fatal_error_handler != NULL) {
		fatal_error_handler(fmsg);
	} else {
		fprintf(stderr, _(PACKAGE_NAME ": FATAL ERROR: %s\n"), fmsg);
	}
	
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
	
	// Otherwise report success
	else {
		cpi_debugf(NULL, _("Dynamic resource %p was registered."), res);
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
			cpi_debugf(NULL, _("Dynamic resource %p was freed."), info);
		}
	} else {
		fprintf(stderr, _("ERROR: Trying to release unknown resource %p in cp_release_info.\n"), info);
	}
	cpi_unlock_framework();
}
