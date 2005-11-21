/*-------------------------------------------------------------------------
 * C-Pluff, a plug-in framework for C
 * Copyright 2005 Johannes Lehtinen
 *-----------------------------------------------------------------------*/

/*
 * C-Pluff core functionality and variables
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdarg.h>
#if CP_THREADS == Posix
#include <pthread.h>
#endif /*CP_THREADS == Posix*/
#include "cpluff.h"
#include "core.h"
#include "kazlib/hash.h"


/* ------------------------------------------------------------------------
 * Static function declarations
 * ----------------------------------------------------------------------*/

/**
 * Cleans up data structures after all plug-in activity has stopped.
 */
static void cleanup(void);

/**
 * Compares pointers.
 * 
 * @param ptr1 the first pointer to be compared
 * @param ptr2 the second pointer to be compared
 * @return zero if the pointers are equal, otherwise non-zero
 */
static int ptr_comp_f(const void *ptr1, const void *ptr2);

/**
 * Produces hash values for pointers.
 * 
 * @param ptr the pointer to be hashed
 * @return the hash value for the pointer
 */
static hash_val_t ptr_hash_f(const void *ptr);

#ifdef CP_THREADS

/**
 * Locks the data mutex.
 */
static void lock_mutex(void);

/**
 * Unlocks the data mutex.
 */
static void unlock_mutex(void);

#endif /*CP_THREADS*/


/* ------------------------------------------------------------------------
 * Variables
 * ----------------------------------------------------------------------*/

/** Installed error handlers */
static hash_t *error_handlers = NULL;

/** Installed event listeners */
static hash_t *event_listeners = NULL;

/* Recursive mutex implementation for data access */
#ifdef CP_THREADS
static pthread_mutex_t data_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t data_cond = PTHREAD_COND_INITIALIZER;
static unsigned int data_lock_count = 0;
static pthread_t data_thread;
#endif /*CP_THREADS*/


/* ------------------------------------------------------------------------
 * Function definitions
 * ----------------------------------------------------------------------*/


/* Generic */

static int ptr_comp_f(const void *ptr1, const void *ptr2) {
	return (ptr1 != ptr2);
}

static hash_val_t ptr_hash_f(const void *ptr) {
	return (hash_val_t) ptr;
}


/* Initialization and destroy */

int cp_init(void) {
	int status = CP_OK;
	
	/* Initialize data structures as necessary */
	if (error_handlers == NULL && status == CP_OK) {
		error_handlers = hash_create(HASHCOUNT_T_MAX, ptr_comp_f, ptr_hash_f);
		if (error_handlers == NULL) {
			status = CP_ERR_RESOURCE;
		}
	}
	if (event_listeners == NULL && status == CP_OK) {
		event_listeners = hash_create(HASHCOUNT_T_MAX, ptr_comp_f, ptr_hash_f);
		if (event_listeners == NULL) {
			status = CP_ERR_RESOURCE;
		}
	}
	
	/* Rollback initialization on failure */
	if (status != CP_OK) {
		cleanup();
	}
	
	/* Return the final status */
	return status;
}

void cp_destroy(void) {
	
	/* Stop and unload all plugins */
	cp_unload_all_plugins();
	
	/* Clean up data structures */
	cleanup();
}

static void cleanup(void) {
	if (error_handlers != NULL) {
		hash_free_nodes(error_handlers);
		hash_destroy(error_handlers);
		error_handlers = NULL;
	}
	if (event_listeners != NULL) {
		hash_free_nodes(event_listeners);
		hash_destroy(event_listeners);
		event_listeners = NULL;
	}
}


/* Error handling */

int cp_add_error_handler(cp_error_handler_t error_handler) {
	int status;
	
	cpi_acquire_data();
	if (hash_lookup(error_handlers, (void *) error_handler) == NULL) {
		if (hash_alloc_insert(error_handlers, (void *) error_handler,
			(void *) error_handler)) {
			status = CP_OK;
		} else {
			status = CP_ERR_RESOURCE;
		}
	} else {
		status = CP_OK;
	}
	cpi_release_data();
	if (status != CP_OK) {
		cpi_process_error("An error handler could not be registered due to insufficient resources.");
	}
	return status;
}

void cp_remove_error_handler(cp_error_handler_t error_handler) {
	hnode_t *node;
	
	cpi_acquire_data();
	node = hash_lookup(error_handlers, (void *) error_handler);
	if (node != NULL) {
		hash_delete_free(error_handlers, node);
	}
	cpi_release_data();
}

void cpi_process_error(const char *msg) {
	hscan_t scan;
	hnode_t *node;
	
	cpi_acquire_data();
	hash_scan_begin(&scan, error_handlers);
	while ((node = hash_scan_next(&scan)) != NULL) {
		((cp_error_handler_t) hnode_get(node))(msg);
	}
	cpi_release_data();
}

void cpi_process_errorf(const char *msg, ...) {
	va_list params;
	char fmsg[256];
	
	va_start(params, msg);
	vsnprintf(fmsg, sizeof(fmsg), msg, params);
	va_end(params);
	fmsg[sizeof(fmsg)/sizeof(char)- 1] = '\0';
	cpi_process_error(fmsg);
}


/* Event listeners */

int cp_add_event_listener(cp_event_listener_t event_listener) {
	int status;
	
	cpi_acquire_data();
	if (hash_lookup(event_listeners, (void *) event_listener) == NULL) {
		if (hash_alloc_insert(event_listeners, (void *) event_listener,
			(void *) event_listener)) {
			status = CP_OK;
		} else {
			status = CP_ERR_RESOURCE;
		}
	} else {
		status = CP_OK;
	}
	cpi_release_data();
	if (status != CP_OK) {
		cpi_process_error("An event listener could not be registered due to insufficient resources.");
	}
	return status;	
}

void cp_remove_event_listener(cp_event_listener_t event_listener) {
	hnode_t *node;
	
	cpi_acquire_data();
	node = hash_lookup(event_listeners, (void *) event_listener);
	if (node != NULL) {
		hash_delete_free(event_listeners, node);
	}
	cpi_release_data();
}

void cpi_deliver_event(const cp_plugin_event_t *event) {
	hscan_t scan;
	hnode_t *node;
	
	cpi_acquire_data();
	hash_scan_begin(&scan, event_listeners);
	while ((node = hash_scan_next(&scan)) != NULL) {
		((cp_event_listener_t) hnode_get(node))(event);
	}
	cpi_release_data();
}


/* Locking */

void cpi_acquire_data(void) {
#if CP_THREADS == Posix
	pthread_t self = pthread_self();
	lock_mutex();
	while (data_lock_count > 0 && !pthread_equal(self, data_thread)) {
		pthread_cond_wait(&data_cond, &data_mutex);
	}
	data_thread = self;
	data_lock_count++;
	unlock_mutex();
#endif /*CP_THREADS == Posix*/
}

void cpi_release_data(void) {
#if CP_THREADS == Posix
	lock_mutex();
	assert(pthread_equal(pthread_self(), data_thread));
	data_lock_count--;
	pthread_cond_broadcast(&data_cond);
	unlock_mutex();
#endif /*CP_THREADS == Posix*/
}

#ifdef CP_THREADS

static void lock_mutex(void) {
	int ec;
	if ((ec = pthread_mutex_lock(&data_mutex))) {
		fprintf(stderr,
			"fatal error: mutex locking failed (error code %d)\n", ec);
		exit(1);
	}
}

static void unlock_mutex(void) {
	int ec;
	if ((ec = pthread_mutex_unlock(&data_mutex))) {
		fprintf(stderr,
			"fatal error: mutex unlocking failed (error code %d)\n", ec);
		exit(1);
	}
}

#endif /*CP_THREADS*/
