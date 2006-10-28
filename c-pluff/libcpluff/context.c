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
 * A holder structure for a plug-in listener.
 */
typedef struct el_holder_t {
	cp_plugin_listener_t plugin_listener;
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
 * Variables
 * ----------------------------------------------------------------------*/

/// Existing contexts
static list_t *contexts = NULL;


/* ------------------------------------------------------------------------
 * Function definitions
 * ----------------------------------------------------------------------*/

// Generic 

/**
 * Processes a node by freeing the associated eh_holder_t and deleting
 * the node from the list.
 * 
 * @param list the list being processed
 * @param node the node being processed
 * @param dummy not used
 */
static void process_free_eh_holder(list_t *list, lnode_t *node, void *dummy) {
	eh_holder_t *h = lnode_get(node);
	list_delete(list, node);
	lnode_destroy(node);
	free(h);
}

/**
 * Processes a node by freeing the associated el_holder_t and deleting the
 * node from the list.
 * 
 * @param list the list being processed
 * @param node the node being processed
 * @param dummy not used
 */
static void process_free_el_holder(list_t *list, lnode_t *node, void *dummy) {
	el_holder_t *h = lnode_get(node);
	list_delete(list, node);
	lnode_destroy(node);
	free(h);
}

/**
 * Compares error handler holders.
 * 
 * @param h1 the first holder to be compared
 * @param h2 the second holder to be compared
 * @return zero if the holders point to the same function, otherwise non-zero
 */
static int comp_eh_holder(const void *h1, const void *h2) {
	const eh_holder_t *ehh1 = h1;
	const eh_holder_t *ehh2 = h2;
	
	return (ehh1->error_handler != ehh2->error_handler);
}

/**
 * Compares plug-in listener holders.
 * 
 * @param h1 the first holder to be compared
 * @param h2 the second holder to be compared
 * @return zero if the holders point to the same function, otherwise non-zero
 */
static int comp_el_holder(const void *h1, const void *h2) {
	const el_holder_t *plh1 = h1;
	const el_holder_t *plh2 = h2;
	
	return (plh1->plugin_listener != plh2->plugin_listener);
}

/**
 * Processes a node by delivering the specified error message to the associated
 * error handler.
 * 
 * @param list the list being processed
 * @param node the node being processed
 * @param msg the error message
 */
static void process_error(list_t *list, lnode_t *node, void *msg) {
	eh_holder_t *h = lnode_get(node);
	h->error_handler(h->context, msg, h->user_data);
}

/**
 * Processes a node by delivering the specified event to the associated
 * plug-in listener.
 * 
 * @param list the list being processed
 * @param node the node being processed
 * @param event the event
 */
static void process_event(list_t *list, lnode_t *node, void *event) {
	el_holder_t *h = lnode_get(node);
	h->plugin_listener(h->context, event, h->user_data);
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
		context->plugin_listeners = list_create(LISTCOUNT_T_MAX);
		context->plugin_dirs = list_create(LISTCOUNT_T_MAX);
		context->plugins = hash_create(HASHCOUNT_T_MAX,
			(int (*)(const void *, const void *)) strcmp, NULL);
		context->started_plugins = list_create(LISTCOUNT_T_MAX);
		if (context->error_handlers == NULL
#ifdef CP_THREADS
			|| context->mutex == NULL
#endif
			|| context->plugin_listeners == NULL
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
		
		// Create a context list, if necessary, and add context to the list
		cpi_lock_framework();
		if (contexts == NULL) {
			if ((contexts = list_create(LISTCOUNT_T_MAX)) == NULL) {
				status = CP_ERR_RESOURCE;
			}
		}
		if (status == CP_OK) {
			lnode_t *node;
			
			if ((node = lnode_create(context)) == NULL) {
				status = CP_ERR_RESOURCE;
			} else {
				list_append(contexts, node);
			}
		}
		cpi_unlock_framework();
		
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

	// Check invocation, although context not locked
	cpi_check_invocation(context, __func__);

	// Remove context from the context list
	cpi_lock_framework();
	if (contexts != NULL) {
		lnode_t *node;
		
		if ((node = list_find(contexts, context, cpi_comp_ptr)) != NULL) {
			list_delete(contexts, node);
			lnode_destroy(node);
		}
	}
	cpi_unlock_framework();

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
	if (context->plugin_listeners != NULL) {
		list_process(context->plugin_listeners, NULL, process_free_el_holder);
		list_destroy(context->plugin_listeners);
		context->plugin_listeners = NULL;
	}
	
	// Release mutex 
#ifdef CP_THREADS
	if (context->mutex != NULL) {
		cpi_destroy_mutex(context->mutex);
	}
#endif

	free(context);
}

void CP_LOCAL cpi_destroy_all_contexts(void) {
	cpi_lock_framework();
	if (contexts != NULL) {
		lnode_t *node;
		
		while ((node = list_last(contexts)) != NULL) {
			cpi_unlock_framework();
			cp_destroy_context(lnode_get(node));
			cpi_lock_framework();
		}
		list_destroy(contexts);
		contexts = NULL;
	}
	cpi_unlock_framework();
}


// Error handling 

int CP_API cp_add_error_handler(cp_context_t *context, cp_error_handler_t error_handler, void *user_data) {
	int status = CP_ERR_RESOURCE;
	eh_holder_t *holder;
	lnode_t *node;
	
	assert(error_handler != NULL);
	
	cpi_check_invocation(context, __func__);
	cpi_lock_context(context);
	if (context->in_error_handler_invocation) {
		cpi_fatalf(_("%s was called from within an error handler invocation."), __func__);
	}
	if ((holder = malloc(sizeof(eh_holder_t))) != NULL) {
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
	if (status != CP_OK) {
		cpi_error(context, _("An error handler could not be registered due to insufficient system resources."));
	}
	return status;
}

void CP_API cp_remove_error_handler(cp_context_t *context, cp_error_handler_t error_handler) {
	eh_holder_t holder;
	lnode_t *node;
	
	cpi_check_invocation(context, __func__);
	holder.error_handler = error_handler;
	cpi_lock_context(context);
	if (context->in_error_handler_invocation) {
		cpi_fatalf(_("%s was called from within an error handler invocation."), __func__);
	}
	node = list_find(context->error_handlers, &holder, comp_eh_holder);
	if (node != NULL) {
		process_free_eh_holder(context->error_handlers, node, NULL);
	}
	cpi_unlock_context(context);
}

void CP_LOCAL cpi_error(cp_context_t *context, const char *msg) {
	assert(msg != NULL);
	cpi_lock_context(context);
	cpi_inc_error_invocation(context);
	list_process(context->error_handlers, (void *) msg, process_error);
	cpi_dec_error_invocation(context);
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


// Plug-in listeners 

int CP_API cp_add_plugin_listener(cp_context_t *context, cp_plugin_listener_t listener, void *user_data) {
	int status = CP_ERR_RESOURCE;
	el_holder_t *holder;
	lnode_t *node;

	assert(listener != NULL);
	
	cpi_check_invocation(context, __func__);
	cpi_lock_context(context);
	if ((holder = malloc(sizeof(el_holder_t))) != NULL) {
		holder->plugin_listener = listener;
		holder->context = context;
		holder->user_data = user_data;
		if ((node = lnode_create(holder)) != NULL) {
			list_append(context->plugin_listeners, node);
			status = CP_OK;
		} else {
			free(holder);
		}
	}
	cpi_unlock_context(context);
	if (status != CP_OK) {
		cpi_error(context, _("A plug-in listener could not be registered due to insufficient system resources."));
	}
	return status;
}

void CP_API cp_remove_plugin_listener(cp_context_t *context, cp_plugin_listener_t listener) {
	el_holder_t holder;
	lnode_t *node;
	
	cpi_check_invocation(context, __func__);
	holder.plugin_listener = listener;
	cpi_lock_context(context);
	node = list_find(context->plugin_listeners, &holder, comp_el_holder);
	if (node != NULL) {
		process_free_el_holder(context->plugin_listeners, node, NULL);
	}
	cpi_unlock_context(context);
}

void CP_LOCAL cpi_deliver_event(cp_context_t *context, const cp_plugin_event_t *event) {
	assert(event != NULL);
	assert(event->plugin_id != NULL);
	cpi_lock_context(context);
	cpi_inc_event_invocation(context);
	list_process(context->plugin_listeners, (void *) event, process_event);
	cpi_dev_event_invocation(context);
	cpi_unlock_context(context);
}


// Plug-in directories 

int CP_API cp_add_plugin_dir(cp_context_t *context, const char *dir) {
	char *d = NULL;
	lnode_t *node = NULL;
	int status = CP_OK;
	
	assert(dir != NULL);
	
	cpi_check_invocation(context, __func__);
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
	
	cpi_check_invocation(context, __func__);
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


// Invocation checking

#ifndef NDEBUG

void CP_LOCAL cpi_check_invocation(cp_context_t *ctx, const char *func) {
	assert(ctx != NULL);
	assert(func != NULL);
	if (ctx->in_error_handler_invocation) {
		cpi_fatalf(_("%s was called from within an error handler invocation."), func);
	}
	if (ctx->in_event_listener_invocation) {
		cpi_fatalf(_("%s was called from within an event listener invocation."), func);
	}
	if (ctx->in_start_func_invocation) {
		cpi_fatalf(_("%s was called from within a start function invocation."), func);
	}
	if (ctx->in_stop_func_invocation) {
		cpi_fatalf(_("%s was called from within a stop function invocation."), func);
	}
}

#endif

#endif
