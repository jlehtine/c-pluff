/*-------------------------------------------------------------------------
 * C-Pluff, a plug-in framework for C
 * Copyright 2006 Johannes Lehtinen
 *-----------------------------------------------------------------------*/

/*
 * Plug-in context implementation
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdarg.h>
#include <string.h>
#include "../kazlib/list.h"
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

/**
 * A holder structure for error handler.
 */
typedef struct eh_holder_t {
	cp_error_handler_t error_handler;
	cp_context_t *context;
	void *user_data;
} eh_holder_t;

/**
 * A holder structure for event listener.
 */
typedef struct el_holder_t {
	cp_event_listener_t event_listener;
	cp_context_t *context;
	void *user_data;
} el_holder_t;

/**
 * A holder structure for error handler parameters.
 */
typedef struct eh_params_t {
	cp_context_t *context;
	char *msg;
} eh_params_t;


/* ------------------------------------------------------------------------
 * Static function declarations
 * ----------------------------------------------------------------------*/

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
static int comp_eh_holder(const void *h1, const void *h2) CP_PURE;

/**
 * Compares event listener holders.
 * 
 * @param h1 the first holder to be compared
 * @param h2 the second holder to be compared
 * @return zero if the holders point to the same function, otherwise non-zero
 */
static int comp_el_holder(const void *h1, const void *h2) CP_PURE;

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
 * @param event the event
 */
static void process_event(list_t *list, lnode_t *node, void *event);


/* ------------------------------------------------------------------------
 * Function definitions
 * ----------------------------------------------------------------------*/


// Generic 

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
	h->error_handler(h->context, msg, h->user_data);
}

static void process_event(list_t *list, lnode_t *node, void *event) {
	el_holder_t *h = lnode_get(node);
	h->event_listener(h->context, event, h->user_data);
}


// Initialization and destroy 

cp_context_t * CP_API cp_create_context(cp_error_handler_t error_handler, void *user_data, int *error) {
	cp_context_t *context = NULL;
	int status = CP_OK;

	// Initialize internal state 
	do {
	
		// Allocate memory for the context 
		if ((context = malloc(sizeof(cp_context_t))) == NULL) {
			status = CP_ERR_RESOURCE;
			break;
		}		
	
		// Initialize data structures as necessary 
		memset(context, 0, sizeof(cp_context_t));
#ifdef CP_THREADS
		context->mutex = cpi_create_mutex();
#endif
		context->error_handlers = list_create(LISTCOUNT_T_MAX);
		context->event_listeners = list_create(LISTCOUNT_T_MAX);
		context->plugin_dirs = list_create(LISTCOUNT_T_MAX);
		context->plugins = hash_create(HASHCOUNT_T_MAX,
			(int (*)(const void *, const void *)) strcmp, NULL);
		context->started_plugins = list_create(LISTCOUNT_T_MAX);
		if (context->error_handlers == NULL
#ifdef CP_THREADS
			|| context->mutex == NULL
#endif
			|| context->event_listeners == NULL
			|| context->plugin_dirs == NULL
			|| context->plugins == NULL
			|| context->started_plugins == NULL) {
			status = CP_ERR_RESOURCE;
			break;
		}

		// Register initial error handler 
		if (error_handler != NULL) {
			if (cp_add_error_handler(context, error_handler, user_data) != CP_OK) {
				status = CP_ERR_RESOURCE;
				break;
			}
		}
		
	} while (0);
	
	// Report failure 
	if (status != CP_OK) {
		cpi_herror(NULL, error_handler, _("Plug-in context could not be created due to insufficient system resources."), user_data);
	}
	
	// Rollback initialization on failure 
	if (status != CP_OK && context != NULL) {
		cp_destroy_context(context);
		context = NULL;
	}

	// Return the final status 
	if (error != NULL) {
		*error = status;
	}
	
	// Return the context (or NULL on failure) 
	return context;
}

void CP_API cp_destroy_context(cp_context_t *context) {

#ifdef CP_THREADS
	assert(context->mutex == NULL || !cpi_is_mutex_locked(context->mutex));
#else
	assert(!context->locked);
#endif

	// Unload all plug-ins 
	if (context->plugins != NULL && !hash_isempty(context->plugins)) {
		cp_uninstall_all_plugins(context);
	}
	
	// Release data structures 
	if (context->plugins != NULL) {
		assert(hash_isempty(context->plugins));
		hash_destroy(context->plugins);
		context->plugins = NULL;
	}
	if (context->started_plugins != NULL) {
		assert(list_isempty(context->started_plugins));
		list_destroy(context->started_plugins);
		context->started_plugins = NULL;
	}
	if (context->plugin_dirs != NULL) {
		list_process(context->plugin_dirs, NULL, cpi_process_free_ptr);
		list_destroy(context->plugin_dirs);
		context->plugin_dirs = NULL;
	}
	if (context->error_handlers != NULL) {
		list_process(context->error_handlers, NULL, process_free_eh_holder);
		list_destroy(context->error_handlers);
		context->error_handlers = NULL;
	}
	if (context->event_listeners != NULL) {
		list_process(context->event_listeners, NULL, process_free_el_holder);
		list_destroy(context->event_listeners);
		context->event_listeners = NULL;
	}
	
	// Release mutex 
#ifdef CP_THREADS
	if (context->mutex != NULL) {
		cpi_destroy_mutex(context->mutex);
	}
#endif

}


// Error handling 

int CP_API cp_add_error_handler(cp_context_t *context, cp_error_handler_t error_handler, void *user_data) {
	int status = CP_ERR_RESOURCE;
	eh_holder_t *holder;
	lnode_t *node;
	
	assert(error_handler != NULL);
	
	cpi_lock_context(context);
	if (context->error_handlers_frozen) {
		status = CP_ERR_DEADLOCK;
	} else if ((holder = malloc(sizeof(eh_holder_t))) != NULL) {
		holder->error_handler = error_handler;
		holder->context = context;
		holder->user_data = user_data;
		if ((node = lnode_create(holder)) != NULL) {
			list_append(context->error_handlers, node);
			status = CP_OK;
		} else {
			free(holder);
		}
	}
	cpi_unlock_context(context);
	switch (status) {
		case CP_OK:
			break;
		case CP_ERR_RESOURCE:
			cpi_error(context, _("An error handler could not be registered due to insufficient system resources."));
			break;
		case CP_ERR_DEADLOCK:
			cpi_error(context, _("An error handler can not be registered from within an error handler invocation."));
			break;
		default:
			cpi_error(context, _("An error handler could not be registered."));
			break;
	}
	return status;
}

int CP_API cp_remove_error_handler(cp_context_t *context, cp_error_handler_t error_handler) {
	eh_holder_t holder;
	lnode_t *node;
	int status = CP_OK;
	
	holder.error_handler = error_handler;
	cpi_lock_context(context);
	if (context->error_handlers_frozen) {
		status = CP_ERR_DEADLOCK;
	} else {
		node = list_find(context->error_handlers, &holder, comp_eh_holder);
		if (node != NULL) {
			process_free_eh_holder(context->error_handlers, node, NULL);
		} else {
			status = CP_ERR_UNKNOWN;
		}
	}
	cpi_unlock_context(context);
	switch (status) {
		case CP_OK:
			break;
		case CP_ERR_DEADLOCK:
			cpi_error(context, _("An error handler can not be removed from within an error handler invocation."));
			break;
		case CP_ERR_UNKNOWN:
			cpi_error(context, _("Could not remove an unknown error handler."));
			break;
		default:
			cpi_error(context, _("Could not remove an error handler."));
			break;
	}
	return status;
}

void CP_LOCAL cpi_error(cp_context_t *context, const char *msg) {
	assert(msg != NULL);
	cpi_lock_context(context);
	context->error_handlers_frozen++;
	list_process(context->error_handlers, (void *) msg, process_error);
	context->error_handlers_frozen--;
	cpi_unlock_context(context);
}

void CP_LOCAL cpi_errorf(cp_context_t *context, const char *msg, ...) {
	va_list params;
	char fmsg[256];
	
	assert(msg != NULL);
	va_start(params, msg);
	vsnprintf(fmsg, sizeof(fmsg), msg, params);
	va_end(params);
	fmsg[sizeof(fmsg)/sizeof(char) - 1] = '\0';
	cpi_error(context, fmsg);
}

void CP_LOCAL cpi_herror(cp_context_t *context, cp_error_handler_t error_handler, void *user_data, const char *msg) {
	assert(msg != NULL);
	if (error_handler != NULL) {
		error_handler(context, msg, user_data);
	}
}

void CP_LOCAL cpi_herrorf(cp_context_t *context, cp_error_handler_t error_handler, void *user_data, const char *msg, ...) {
	va_list params;
	char fmsg[256];
	
	assert(msg != NULL);
	va_start(params, msg);
	vsnprintf(fmsg, sizeof(fmsg), msg, params);
	va_end(params);
	fmsg[sizeof(fmsg)/sizeof(char) - 1] = '\0';
	cpi_herror(context, error_handler, user_data, msg);
}


// Event listeners 

int CP_API cp_add_event_listener(cp_context_t *context, cp_event_listener_t event_listener, void *user_data) {
	int status = CP_ERR_RESOURCE;
	el_holder_t *holder;
	lnode_t *node;

	assert(event_listener != NULL);
	
	cpi_lock_context(context);
	if (context->event_listeners_frozen) {
		status = CP_ERR_DEADLOCK;
	} else if ((holder = malloc(sizeof(el_holder_t))) != NULL) {
		holder->event_listener = event_listener;
		holder->context = context;
		holder->user_data = user_data;
		if ((node = lnode_create(holder)) != NULL) {
			list_append(context->event_listeners, node);
			status = CP_OK;
		} else {
			free(holder);
		}
	}
	cpi_unlock_context(context);
	switch (status) {
		case CP_OK:
			break;
		case CP_ERR_RESOURCE:
			cpi_error(context, _("An event listener could not be registered due to insufficient system resources."));
			break;
		case CP_ERR_DEADLOCK:
			cpi_error(context, _("An event listener can not be registered from within an event listener invocation."));
			break;
		default:
			cpi_error(context, _("An event listener could not be registered."));
			break;
	}
	return status;
}

int CP_API cp_remove_event_listener(cp_context_t *context, cp_event_listener_t event_listener) {
	el_holder_t holder;
	lnode_t *node;
	int status = CP_OK;
	
	holder.event_listener = event_listener;
	cpi_lock_context(context);
	if (context->event_listeners_frozen) {
		status = CP_ERR_DEADLOCK;
	} else {
		node = list_find(context->event_listeners, &holder, comp_el_holder);
		if (node != NULL) {
			process_free_el_holder(context->event_listeners, node, NULL);
		} else {
			status = CP_ERR_UNKNOWN;
		}
	}
	cpi_unlock_context(context);
	switch (status) {
		case CP_OK:
			break;
		case CP_ERR_DEADLOCK:
			cpi_error(context, _("An event listener can not be removed from within an event listener invocation."));
			break;
		case CP_ERR_UNKNOWN:
			cpi_error(context, _("Could not remove an unknown event listener."));
			break;
		default:
			cpi_error(context, _("Could not remove an event listener."));
			break;
	}
	return status;
}

void CP_LOCAL cpi_deliver_event(cp_context_t *context, const cp_plugin_event_t *event) {
	assert(event != NULL);
	assert(event->plugin_id != NULL);
	cpi_lock_context(context);
	context->event_listeners_frozen++;
	list_process(context->event_listeners, (void *) event, process_event);
	context->event_listeners_frozen--;
	cpi_unlock_context(context);
}


// Plug-in directories 

int CP_API cp_add_plugin_dir(cp_context_t *context, const char *dir) {
	char *d = NULL;
	lnode_t *node = NULL;
	int status = CP_OK;
	
	assert(dir != NULL);
	
	cpi_lock_context(context);
	do {
	
		// Check if directory has already been registered 
		if (list_find(context->plugin_dirs, dir, (int (*)(const void *, const void *)) strcmp) != NULL) {
			break;
		}
	
		// Allocate resources 
		d = malloc(sizeof(char) * (strlen(dir) + 1));
		node = lnode_create(d);
		if (d == NULL || node == NULL) {
			status = CP_ERR_RESOURCE;
			break;
		}
	
		// Register directory 
		strcpy(d, dir);
		list_append(context->plugin_dirs, node);
		
	} while (0);
	cpi_unlock_context(context);

	// Release resources on failure 
	if (status != CP_OK) {	
		if (d != NULL) {
			free(d);
		}
		if (node != NULL) {
			lnode_destroy(node);
		}
	}
	
	// Report errors 
	switch (status) {
		case CP_OK:
			break;
		case CP_ERR_RESOURCE:
			cpi_errorf(context, _("Could not add plug-in directory %s due to insufficient system resources."), dir);
			break;
		default:
			cpi_errorf(context, _("Could not add plug-in directory %s."), dir);
			break;
	}

	return status;
}

void CP_API cp_remove_plugin_dir(cp_context_t *context, const char *dir) {
	char *d;
	lnode_t *node;
	
	assert(dir != NULL);
	
	cpi_lock_context(context);
	node = list_find(context->plugin_dirs, dir, (int (*)(const void *, const void *)) strcmp);
	if (node != NULL) {
		d = lnode_get(node);
		list_delete(context->plugin_dirs, node);
		lnode_destroy(node);
		free(d);
	}
	cpi_unlock_context(context);
}


// Locking 

#if defined(CP_THREADS) || !defined(NDEBUG)

void CP_LOCAL cpi_lock_context(cp_context_t *context) {
#if defined(CP_THREADS)
	cpi_lock_mutex(context->mutex);
#elif !defined(NDEBUG)
	context->locked++;
#endif
}

void CP_LOCAL cpi_unlock_context(cp_context_t *context) {
#if defined(CP_THREADS)
	cpi_unlock_mutex(context->mutex);
#elif !defined(NDEBUG)
	assert(context->locked > 0);
	context->locked--;
#endif
}

#endif
