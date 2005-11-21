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
#ifdef CP_THREADS_POSIX
#include <pthread.h>
#endif /*CP_THREADS_POSIX*/
#include "cpluff.h"
#include "core.h"
#include "kazlib/list.h"

/* ------------------------------------------------------------------------
 * Data types
 * ----------------------------------------------------------------------*/

/**
 * A holder structure for error handler pointers. For ISO C conformance
 * (to avoid conversions between function and object pointers).
 */
typedef struct eh_holder_t {
	cp_error_handler_t error_handler;
} eh_holder_t;

/**
 * A holder structure for event listener pointers. For ISO C conformance
 * (to avoid conversions between function and object pointers).
 */
typedef struct el_holder_t {
	cp_event_listener_t event_listener;
} el_holder_t;


/* ------------------------------------------------------------------------
 * Static function declarations
 * ----------------------------------------------------------------------*/

/**
 * Cleans up data structures after all plug-in activity has stopped.
 */
static void cleanup(void);

/**
 * Processes a node by freeing the associated eh_holder_t and deleting
 * the node from the list.
 * 
 * @param list the list being processed
 * @param node the node being processed
 * @param dummy not used
 */
static void process_free_eh_holder(list_t *list, lnode_t *node, void *dummy);

/**
 * Processes a node by freeing the associated el_holder_t and deleting the
 * node from the list.
 * 
 * @param list the list being processed
 * @param node the node being processed
 * @param dummy not used
 */
static void process_free_el_holder(list_t *list, lnode_t *node, void *dummy);

/**
 * Compares error handler holders.
 * 
 * @param h1 the first holder to be compared
 * @param h2 the second holder to be compared
 * @return zero if the holders point to the same function, otherwise non-zero
 */
static int comp_eh_holder(const void *h1, const void *h2);

/**
 * Compares event listener holders.
 * 
 * @param h1 the first holder to be compared
 * @param h2 the second holder to be compared
 * @return zero if the holders point to the same function, otherwise non-zero
 */
static int comp_el_holder(const void *h1, const void *h2);

/**
 * Processes a node by delivering the specified error message to the associated
 * error handler.
 * 
 * @param list the list being processed
 * @param node the node being processed
 * @param msg the error message
 */
static void process_error(list_t *list, lnode_t *node, void *msg);

/**
 * Processes a node by delivering the specified event to the associated
 * event listener.
 * 
 * @param list the list being processed
 * @param node the node being processed
 * @param event the event being delivered
 */
static void process_event(list_t *list, lnode_t *node, void *event);


#ifdef CP_THREADS_POSIX

/**
 * Locks the data mutex.
 */
static void lock_mutex(void);

/**
 * Unlocks the data mutex.
 */
static void unlock_mutex(void);

#endif /*CP_THREADS_POSIX*/


/* ------------------------------------------------------------------------
 * Variables
 * ----------------------------------------------------------------------*/

/** Installed error handlers */
static list_t *error_handlers = NULL;

/** Installed event listeners */
static list_t *event_listeners = NULL;

/* Recursive mutex implementation for data access */
#ifdef CP_THREADS_POSIX
static pthread_mutex_t data_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t data_cond = PTHREAD_COND_INITIALIZER;
static unsigned int data_lock_count = 0;
static pthread_t data_thread;
#endif /*CP_THREADS_POSIX*/


/* ------------------------------------------------------------------------
 * Function definitions
 * ----------------------------------------------------------------------*/


/* Generic */

static void process_free_eh_holder(list_t *list, lnode_t *node, void *dummy) {
	eh_holder_t *h = lnode_get(node);
	list_delete(list, node);
	lnode_destroy(node);
	free(h);
}

static void process_free_el_holder(list_t *list, lnode_t *node, void *dummy) {
	el_holder_t *h = lnode_get(node);
	list_delete(list, node);
	lnode_destroy(node);
	free(h);
}

static int comp_eh_holder(const void *h1, const void *h2) {
	const eh_holder_t *ehh1 = h1;
	const eh_holder_t *ehh2 = h2;
	
	return (ehh1->error_handler != ehh2->error_handler);
}

static int comp_el_holder(const void *h1, const void *h2) {
	const el_holder_t *elh1 = h1;
	const el_holder_t *elh2 = h2;
	
	return (elh1->event_listener != elh2->event_listener);
}

static void process_error(list_t *list, lnode_t *node, void *msg) {
	eh_holder_t *h = lnode_get(node);
	h->error_handler(msg);
}

static void process_event(list_t *list, lnode_t *node, void *event) {
	el_holder_t *h = lnode_get(node);
	h->event_listener(event);
}


/* Initialization and destroy */

CP_EXPORT int cp_init(void) {
	int status = CP_OK;
	
	/* Initialize data structures as necessary */
	if (error_handlers == NULL && status == CP_OK) {
		error_handlers = list_create(LISTCOUNT_T_MAX);
		if (error_handlers == NULL) {
			status = CP_ERR_RESOURCE;
		}
	}
	if (event_listeners == NULL && status == CP_OK) {
		event_listeners = list_create(LISTCOUNT_T_MAX);
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

CP_EXPORT void cp_destroy(void) {
	
	/* Stop and unload all plugins */
	cp_unload_all_plugins();
	
	/* Clean up data structures */
	cleanup();
}

static void cleanup(void) {
	if (error_handlers != NULL) {
		list_process(error_handlers, NULL, process_free_eh_holder);
		list_destroy(error_handlers);
		error_handlers = NULL;
	}
	if (event_listeners != NULL) {
		list_process(error_handlers, NULL, process_free_el_holder);
		list_destroy(event_listeners);
		event_listeners = NULL;
	}
}


/* Error handling */

CP_EXPORT int cp_add_error_handler(cp_error_handler_t error_handler) {
	int status = CP_ERR_RESOURCE;
	eh_holder_t *holder;
	lnode_t *node;
	
	cpi_acquire_data();
	if ((holder = malloc(sizeof(eh_holder_t))) != NULL) {
		holder->error_handler = error_handler;
		if ((node = lnode_create(holder)) != NULL) {
			list_append(error_handlers, node);
			status = CP_OK;
		} else {
			free(holder);
		}
	}
	cpi_release_data();
	if (status != CP_OK) {
		cpi_error("An error handler could not be registered due to insufficient resources.");
	}
	return status;
}

CP_EXPORT void cp_remove_error_handler(cp_error_handler_t error_handler) {
	eh_holder_t holder;
	lnode_t *node;
	
	holder.error_handler = error_handler;
	cpi_acquire_data();
	node = list_find(error_handlers, &holder, comp_eh_holder);
	if (node != NULL) {
		process_free_eh_holder(error_handlers, node, NULL);
	}
	cpi_release_data();
}

void cpi_error(const char *msg) {
	cpi_acquire_data();
	list_process(error_handlers, (void *) msg, process_error);
	cpi_release_data();
}

void cpi_errorf(const char *msg, ...) {
	va_list params;
	char fmsg[256];
	
	va_start(params, msg);
	vsnprintf(fmsg, sizeof(fmsg), msg, params);
	va_end(params);
	fmsg[sizeof(fmsg)/sizeof(char)- 1] = '\0';
	cpi_error(fmsg);
}


/* Event listeners */

CP_EXPORT int cp_add_event_listener(cp_event_listener_t event_listener) {
	int status = CP_ERR_RESOURCE;
	el_holder_t *holder;
	lnode_t *node;
	
	cpi_acquire_data();
	if ((holder = malloc(sizeof(el_holder_t))) != NULL) {
		holder->event_listener = event_listener;
		if ((node = lnode_create(holder)) != NULL) {
			list_append(event_listeners, node);
			status = CP_OK;
		} else {
			free(holder);
		}
	}
	cpi_release_data();
	if (status != CP_OK) {
		cpi_error("An event listener could not be registered due to insufficient resources.");
	}
	return status;
}

CP_EXPORT void cp_remove_event_listener(cp_event_listener_t event_listener) {
	el_holder_t holder;
	lnode_t *node;
	
	holder.event_listener = event_listener;
	cpi_acquire_data();
	node = list_find(event_listeners, &holder, comp_el_holder);
	if (node != NULL) {
		process_free_el_holder(event_listeners, node, NULL);
	}
	cpi_release_data();
}

void cpi_deliver_event(const cp_plugin_event_t *event) {
	cpi_acquire_data();
	list_process(event_listeners, (void *) event, process_event);
	cpi_release_data();
}


/* Locking */

void cpi_acquire_data(void) {
#ifdef CP_THREADS_POSIX
	pthread_t self = pthread_self();
	lock_mutex();
	while (data_lock_count > 0 && !pthread_equal(self, data_thread)) {
		pthread_cond_wait(&data_cond, &data_mutex);
	}
	data_thread = self;
	data_lock_count++;
	unlock_mutex();
#endif /*CP_THREADS_POSIX*/
}

void cpi_release_data(void) {
#ifdef CP_THREADS_POSIX
	lock_mutex();
	assert(pthread_equal(pthread_self(), data_thread));
	data_lock_count--;
	pthread_cond_broadcast(&data_cond);
	unlock_mutex();
#endif /*CP_THREADS_POSIX*/
}

#ifdef CP_THREADS_POSIX

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

#endif /*CP_THREADS_POSIX*/
