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
 * Processes a node by delivering the specified event to the associated
 * plug-in listener.
 * 
 * @param list the list being processed
 * @param node the node being processed
 * @param event the event
 */
static void process_event(list_t *list, lnode_t *node, void *event) {
	el_holder_t *h = lnode_get(node);
	cpi_plugin_event_t *e = event;
	h->plugin_listener(h->context, e->plugin_id, e->old_state, e->new_state, h->user_data);
}


// Initialization and destroy 

cp_context_t * CP_API cp_create_context(int *error) {
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
		context->plugin_listeners = list_create(LISTCOUNT_T_MAX);
		context->plugin_dirs = list_create(LISTCOUNT_T_MAX);
		context->plugins = hash_create(HASHCOUNT_T_MAX,
			(int (*)(const void *, const void *)) strcmp, NULL);
		context->started_plugins = list_create(LISTCOUNT_T_MAX);
		context->ext_points = hash_create(HASHCOUNT_T_MAX,
			(int (*)(const void *, const void *)) strcmp, NULL);
		context->extensions = hash_create(HASHCOUNT_T_MAX,
			(int (*)(const void *, const void *)) strcmp, NULL);
		if (context->plugin_listeners == NULL
#ifdef CP_THREADS
			|| context->mutex == NULL
#endif
			|| context->plugin_dirs == NULL
			|| context->plugins == NULL
			|| context->started_plugins == NULL
			|| context->ext_points == NULL
			|| context->extensions == NULL) {
			status = CP_ERR_RESOURCE;
			break;
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
	
	// Report failure or success
	if (status != CP_OK) {
		cpi_error(NULL, _("Plug-in context could not be created due to insufficient system resources."));
	} else {
		cpi_debugf(NULL, "Plug-in context %p was created.", (void *) context);
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
	assert(context != NULL);
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
	if (context->plugin_listeners != NULL) {
		list_process(context->plugin_listeners, NULL, process_free_el_holder);
		list_destroy(context->plugin_listeners);
		context->plugin_listeners = NULL;
	}
	if (context->ext_points != NULL) {
		assert(hash_isempty(context->ext_points));
		hash_destroy(context->ext_points);
	}
	if (context->extensions != NULL) {
		assert(hash_isempty(context->extensions));
		hash_destroy(context->extensions);
	}
	
	// Release mutex 
#ifdef CP_THREADS
	if (context->mutex != NULL) {
		cpi_destroy_mutex(context->mutex);
	}
#endif

	// Release context data structure
	free(context);
	
	// Log event
	cpi_debugf(NULL, "Plug-in context %p was destroyed.", (void *) context);
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


// Plug-in listeners 

int CP_API cp_add_plugin_listener(cp_context_t *context, cp_plugin_listener_t listener, void *user_data) {
	int status = CP_ERR_RESOURCE;
	el_holder_t *holder;
	lnode_t *node;

	assert(context != NULL);
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
	} else {
		cpi_debugf(context, "Plug-in listener %p was added.", (void *) listener);
	}
	return status;
}

void CP_API cp_remove_plugin_listener(cp_context_t *context, cp_plugin_listener_t listener) {
	el_holder_t holder;
	lnode_t *node;
	
	assert(context != NULL);
	cpi_check_invocation(context, __func__);
	holder.plugin_listener = listener;
	cpi_lock_context(context);
	node = list_find(context->plugin_listeners, &holder, comp_el_holder);
	if (node != NULL) {
		process_free_el_holder(context->plugin_listeners, node, NULL);
	}
	cpi_unlock_context(context);
	cpi_debugf(context, "Plug-in listener %p was removed.", (void *) listener);
}

void CP_LOCAL cpi_deliver_event(cp_context_t *context, const cpi_plugin_event_t *event) {
	assert(event != NULL);
	assert(event->plugin_id != NULL);
	cpi_lock_context(context);
	cpi_inc_event_invocation(context);
	list_process(context->plugin_listeners, (void *) event, process_event);
	cpi_dev_event_invocation(context);
	cpi_unlock_context(context);
	if (cpi_is_logged(CP_LOG_INFO)) {
		char *str;
		switch (event->new_state) {
			case CP_PLUGIN_UNINSTALLED:
				str = _("Plug-in %s has been uninstalled.");
				break;
			case CP_PLUGIN_INSTALLED:
				if (event->old_state < CP_PLUGIN_INSTALLED) {
					str = _("Plug-in %s has been installed.");
				} else {
					str = _("Plug-in %s runtime has been unloaded.");
				}
				break;
			case CP_PLUGIN_RESOLVED:
				if (event->old_state < CP_PLUGIN_RESOLVED) {
					str = _("Plug-in %s dependencies have been resolved and the plug-in runtime has been loaded.");
				} else {
					str = _("Plug-in %s has been stopped.");
				}
				break;
			case CP_PLUGIN_STARTING:
				str = _("Plug-in %s is starting.");
				break;
			case CP_PLUGIN_STOPPING:
				str = _("Plug-in %s is stopping.");
				break;
			case CP_PLUGIN_ACTIVE:
				str = _("Plug-in %s has been started.");
				break;
			default:
				str = NULL;
				break;
		}
		if (str != NULL) {
			cpi_infof(context, str, event->plugin_id);
		}
	}
}


// Plug-in directories 

int CP_API cp_add_plugin_dir(cp_context_t *context, const char *dir) {
	char *d = NULL;
	lnode_t *node = NULL;
	int status = CP_OK;
	
	assert(context != NULL);
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

	// Report success
	if (status == CP_OK) {
		cpi_debugf(context, "Plug-in directory %s was added.", dir);
	}
	
	return status;
}

void CP_API cp_remove_plugin_dir(cp_context_t *context, const char *dir) {
	char *d;
	lnode_t *node;
	
	assert(context != NULL);
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
	cpi_debugf(context, "Plug-in directory %s was removed.", dir);
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

void CP_LOCAL cpi_check_invocation(cp_context_t *ctx, const char *func) {
	assert(ctx != NULL);
	assert(func != NULL);
	if (ctx->in_logger_invocation) {
		cpi_fatalf(_("%s was called from within a logger invocation."), func);
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
