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
#ifdef HAVE_PTHREAD_H
#include <pthread.h>
#endif /*HAVE_PTHREAD_H*/
#include "cpluff.h"
#include "core.h"


/* ------------------------------------------------------------------------
 * Static function declarations
 * ----------------------------------------------------------------------*/

/**
 * Cleans up data structures after all plug-in activity has stopped.
 */
static void cleanup(void);

/**
 * Adds a data pointer to the specified set if it is not already included.
 * 
 * @param set the set implemented as a list
 * @param data the data pointer
 * @return CPLUFF_OK (0) on success, CPLUFF_ERROR (-1) on failure
 */
static int set_append(list_t *set, void *data);

/**
 * Removes a data pointer from the specified set if it is included.
 * 
 * @param set the set implemented as a list
 * @param data the data pointer
 */
static void set_remove(list_t *set, void *data);

/**
 * Compares pointers.
 */
static int compare_ptr(const void *ptr1, const void *ptr2);

/**
 * Delivers an error message to an error handler associated with a node.
 * 
 * @param list the error handlers list
 * @param node the node being processed
 * @param msg the error message
 */
static void process_error(list_t *list, lnode_t *node, void *msg);

/**
 * Delivers an event to an event listener associated with a node.
 * 
 * @param list the event listeners list
 * @param node the node being processed
 * @param event the event being delivered
 */
static void deliver_event(list_t *list, lnode_t *node, void *event);

/**
 * Locks the data mutex.
 */
static void lock_mutex(void);

/**
 * Unlocks the data mutex.
 */
static void unlock_mutex(void);


/* ------------------------------------------------------------------------
 * Variables
 * ----------------------------------------------------------------------*/

list_t *plugins = NULL;

/** Installed error handlers */
static list_t *error_handlers = NULL;

/** Installed event listeners */
static list_t *event_listeners = NULL;

/* Recursive mutex implementation for data access */
#ifdef HAVE_PTHREAD_H
static pthread_mutex_t data_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t data_cond = PTHREAD_COND_INITIALIZER;
static unsigned int data_lock_count = 0;
static pthread_t data_thread;
#endif /*HAVE_PTHREAD_H*/


/* ------------------------------------------------------------------------
 * Function definitions
 * ----------------------------------------------------------------------*/


/* Generic */

static int set_append(list_t *set, void *data) {
	int status = CPLUFF_OK;
	if (list_find(set, data, compare_ptr) == NULL) {
		lnode_t *node = lnode_create(data);
		if (node != NULL) {
			list_append(set, node);
		} else {
			status = CPLUFF_ERROR;
		}
	}
	return status;
}

static void set_remove(list_t *set, void *data) {
	lnode_t *node;
	node = list_find(set, data, compare_ptr);
	if (node != NULL) {
		list_delete(set, node);
		lnode_destroy(node);
	}
}

static int compare_ptr(const void *ptr1, const void *ptr2) {
	return (ptr1 == ptr2 ? 0 : 1);
}


/* Initialization and destroy */

int cpluff_init(void) {
	int status = CPLUFF_OK;
	
	/* Initialize data structures as necessary */
	if (plugins == NULL && status == CPLUFF_OK) {
		plugins = list_create(LISTCOUNT_T_MAX);
		if (!plugins) {
			status = CPLUFF_ERROR;
		}
	}
	if (error_handlers == NULL && status == CPLUFF_OK) {
		error_handlers = list_create(LISTCOUNT_T_MAX);
		if (error_handlers == NULL) {
			status = CPLUFF_ERROR;
		}
	}
	if (event_listeners == NULL && status == CPLUFF_OK) {
		event_listeners = list_create(LISTCOUNT_T_MAX);
		if (event_listeners == NULL) {
			status = CPLUFF_ERROR;
		}
	}
	
	/* Rollback initialization on failure */
	if (status != CPLUFF_OK) {
		cleanup();
	}
	
	/* Return the final status */
	return status;
}

void cpluff_destroy(void) {
	
	/* Stop and unload all plugins */
	unload_all_plugins();
	
	/* Clean up data structures */
	cleanup();
}

static void cleanup(void) {
	if (plugins != NULL) {
		list_destroy(plugins);
		plugins = NULL;
	}
	if (error_handlers != NULL) {
		list_destroy_nodes(error_handlers);
		list_destroy(error_handlers);
		error_handlers = NULL;
	}
	if (event_listeners != NULL) {
		list_destroy_nodes(event_listeners);
		list_destroy(event_listeners);
		event_listeners = NULL;
	}
}


/* Error handling */

int add_cp_error_handler(void (*error_handler)(const char *msg)) {
	int status;
	acquire_cp_data();
	status = set_append(error_handlers, (void *) error_handler);
	release_cp_data();
	if (status != CPLUFF_OK) {
		cpi_process_error("An error handler could not be registered due to insufficient resources.");
	}
	return status;
}

void remove_cp_error_handler(void (*error_handler)(const char *msg)) {
	acquire_cp_data();
	set_remove(error_handlers, (void *) error_handler);
	release_cp_data();
}

void cpi_process_error(const char *msg) {
	acquire_cp_data();
	list_process(error_handlers, (void *) msg, process_error);
	release_cp_data();
}

static void process_error(list_t *list, lnode_t *node, void *msg) {
	void (*handler)(const char *) = (void (*)(const char *)) lnode_get(node);
	handler((const char *) msg);
}


/* Event listeners */

int add_cp_event_listener
	(void (*event_listener)(const plugin_event_t *event)) {
	int status;
	acquire_cp_data();
	status = set_append(event_listeners, (void *) event_listener);
	release_cp_data();
	if (status != CPLUFF_OK) {
		cpi_process_error("An event listener could not be registered due to insufficient resources.");
	}
	return status;	
}

void remove_cp_event_listener
	(void (*event_listener)(const plugin_event_t *event)) {
	acquire_cp_data();
	set_remove(event_listeners, (void *) event_listener);
	release_cp_data();
}

void cpi_deliver_event(const plugin_event_t *event) {
	acquire_cp_data();
	list_process(event_listeners, (void *) event, deliver_event);
	release_cp_data();
}

static void deliver_event(list_t *list, lnode_t *node, void *event) {
	void (*listener)(const plugin_event_t *)
		= (void (*)(const plugin_event_t *)) lnode_get(node);
	listener((const plugin_event_t *) event);
}


/* Locking */

void acquire_cp_data(void) {
#ifdef HAVE_PTHREAD_H
	pthread_t self = pthread_self();
	lock_mutex();
	while (data_lock_count > 0 && !pthread_equal(self, data_thread)) {
		pthread_cond_wait(&data_cond, &data_mutex);
	}
	data_thread = self;
	data_lock_count++;
	unlock_mutex();
#endif /*HAVE_PTHREAD_H*/
}

void release_cp_data(void) {
#ifdef HAVE_PTHREAD_H
	lock_mutex();
	assert(pthread_equal(pthread_self(), data_thread));
	data_lock_count--;
	pthread_cond_broadcast(&data_cond);
	unlock_mutex();
#endif /*HAVE_PTHREAD_H*/
}

#ifdef HAVE_PTHREAD_H

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

#endif /*HAVE_PTHREAD_H*/
