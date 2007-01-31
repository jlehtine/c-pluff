/*-------------------------------------------------------------------------
 * C-Pluff, a plug-in framework for C
 * Copyright 2006 Johannes Lehtinen
 *-----------------------------------------------------------------------*/

/** @file
 * Plug-in context implementation
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdarg.h>
#include <string.h>
#include "../kazlib/list.h"
#include "cpluff.h"
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
	cp_plugin_listener_func_t plugin_listener;
	void *user_data;
} el_holder_t;


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
	h->plugin_listener(e->plugin_id, e->old_state, e->new_state, h->user_data);
}

static void free_plugin_env(cp_plugin_env_t *env) {
	assert(env != NULL);
	
	// Free environment data
	if (env->plugins != NULL) {
		assert(hash_isempty(env->plugins));
		hash_destroy(env->plugins);
		env->plugins = NULL;
	}
	if (env->started_plugins != NULL) {
		assert(list_isempty(env->started_plugins));
		list_destroy(env->started_plugins);
		env->started_plugins = NULL;
	}
	if (env->plugin_dirs != NULL) {
		list_process(env->plugin_dirs, NULL, cpi_process_free_ptr);
		list_destroy(env->plugin_dirs);
		env->plugin_dirs = NULL;
	}
	if (env->plugin_listeners != NULL) {
		list_process(env->plugin_listeners, NULL, process_free_el_holder);
		list_destroy(env->plugin_listeners);
		env->plugin_listeners = NULL;
	}
	if (env->loggers != NULL) {
		cpi_unregister_loggers(env->loggers, NULL);
		env->loggers = NULL;
	}
	if (env->ext_points != NULL) {
		assert(hash_isempty(env->ext_points));
		hash_destroy(env->ext_points);
	}
	if (env->extensions != NULL) {
		assert(hash_isempty(env->extensions));
		hash_destroy(env->extensions);
	}
	
	// Destroy mutex 
#ifdef CP_THREADS
	if (env->mutex != NULL) {
		cpi_destroy_mutex(env->mutex);
	}
#endif

	// Free environment
	free(env);

}

CP_HIDDEN void cpi_free_context(cp_context_t *context) {
	assert(context != NULL);
	
	// Free environment if this is the client program context
	if (context->plugin == NULL && context->env != NULL) {
		free_plugin_env(context->env);
	}
	
	// Destroy symbol lists
	assert(context->resolved_symbols == NULL
			|| hash_isempty(context->resolved_symbols));
	hash_destroy(context->resolved_symbols);
	assert(context->symbol_providers == NULL
			|| hash_isempty(context->symbol_providers));
	hash_destroy(context->symbol_providers);

	// Free context
	free(context);	
}

CP_HIDDEN cp_context_t * cpi_new_context(cp_plugin_t *plugin, cp_plugin_env_t *env, cp_status_t *error) {
	cp_context_t *context = NULL;
	cp_status_t status = CP_OK;
	
	assert(env != NULL);
	assert(error != NULL);
	
	do {
		
		// Allocate memory for the context
		if ((context = malloc(sizeof(cp_context_t))) == NULL) {
			status = CP_ERR_RESOURCE;
			break;
		}
		
		// Initialize context
		context->plugin = plugin;
		context->env = env;
		context->resolved_symbols = NULL;
		context->symbol_providers = NULL;
		
	} while (0);
	
	// Free context on error
	if (status != CP_OK && context != NULL) {
		free(context);
		context = NULL;
	}
	
	*error = status;
	return context;
}

CP_C_API cp_context_t * cp_create_context(cp_status_t *error) {
	cp_plugin_env_t *env = NULL;
	cp_context_t *context = NULL;
	cp_status_t status = CP_OK;

	// Initialize internal state
	do {
	
		// Allocate memory for the plug-in environment
		if ((env = malloc(sizeof(cp_plugin_env_t))) == NULL) {
			status = CP_ERR_RESOURCE;
			break;
		}
	
		// Initialize plug-in environment
		memset(env, 0, sizeof(cp_plugin_env_t));
#ifdef CP_THREADS
		env->mutex = cpi_create_mutex();
#endif
		env->argc = 0;
		env->argv = NULL;
		env->plugin_listeners = list_create(LISTCOUNT_T_MAX);
		env->loggers = list_create(LISTCOUNT_T_MAX);
		env->log_min_severity = CP_LOG_NONE;
		env->plugin_dirs = list_create(LISTCOUNT_T_MAX);
		env->plugins = hash_create(HASHCOUNT_T_MAX,
			(int (*)(const void *, const void *)) strcmp, NULL);
		env->started_plugins = list_create(LISTCOUNT_T_MAX);
		env->ext_points = hash_create(HASHCOUNT_T_MAX,
			(int (*)(const void *, const void *)) strcmp, NULL);
		env->extensions = hash_create(HASHCOUNT_T_MAX,
			(int (*)(const void *, const void *)) strcmp, NULL);
		env->run_funcs = list_create(LISTCOUNT_T_MAX);
		env->run_wait = NULL;
		if (env->plugin_listeners == NULL
			|| env->loggers == NULL
#ifdef CP_THREADS
			|| env->mutex == NULL
#endif
			|| env->plugin_dirs == NULL
			|| env->plugins == NULL
			|| env->started_plugins == NULL
			|| env->ext_points == NULL
			|| env->extensions == NULL) {
			status = CP_ERR_RESOURCE;
			break;
		}
		
		// Create the plug-in context
		if ((context = cpi_new_context(NULL, env, &status)) == NULL) {
			break;
		}
		env = NULL;

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
	
	// Release resources on failure 
	if (status != CP_OK) {
		if (env != NULL) {
			free_plugin_env(env);
		}
		if (context != NULL) {
			cpi_free_context(context);
		}
		context = NULL;
	}

	// Return the final status 
	if (error != NULL) {
		*error = status;
	}
	
	// Return the context (or NULL on failure) 
	return context;
}

CP_C_API void cp_destroy_context(cp_context_t *context) {
	CHECK_NOT_NULL(context);
	if (context->plugin != NULL) {
		cpi_fatalf(_("Only the client program can destroy a plug-in context."));
	}

	// Check invocation
	cpi_lock_context(context);
	cpi_check_invocation(context, CPI_CF_ANY, __func__);
	cpi_unlock_context(context);

#ifdef CP_THREADS
	assert(context->env->mutex == NULL || !cpi_is_mutex_locked(context->env->mutex));
#else
	assert(!context->env->locked);
#endif

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
	cp_uninstall_plugins(context);
	
	// Free context
	cpi_free_context(context);
}

CP_HIDDEN void cpi_destroy_all_contexts(void) {
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

CP_C_API cp_status_t cp_register_plistener(cp_context_t *context, cp_plugin_listener_func_t listener, void *user_data) {
	cp_status_t status = CP_ERR_RESOURCE;
	el_holder_t *holder;
	lnode_t *node;

	CHECK_NOT_NULL(context);
	CHECK_NOT_NULL(listener);
	
	cpi_lock_context(context);
	cpi_check_invocation(context, CPI_CF_LOGGER | CPI_CF_LISTENER, __func__);
	if ((holder = malloc(sizeof(el_holder_t))) != NULL) {
		holder->plugin_listener = listener;
		holder->user_data = user_data;
		if ((node = lnode_create(holder)) != NULL) {
			list_append(context->env->plugin_listeners, node);
			status = CP_OK;
		} else {
			free(holder);
		}
	}
	cpi_unlock_context(context);
	if (status != CP_OK) {
		cpi_error(context, _("A plug-in listener could not be registered due to insufficient system resources."));
	} else {
		cpi_debugf(context, "A plug-in listener was added by %s.", cpi_context_owner(context));
	}
	return status;
}

CP_C_API void cp_unregister_plistener(cp_context_t *context, cp_plugin_listener_func_t listener) {
	el_holder_t holder;
	lnode_t *node;
	
	CHECK_NOT_NULL(context);
	holder.plugin_listener = listener;
	cpi_lock_context(context);
	cpi_check_invocation(context, CPI_CF_LOGGER | CPI_CF_LISTENER, __func__);
	node = list_find(context->env->plugin_listeners, &holder, comp_el_holder);
	if (node != NULL) {
		process_free_el_holder(context->env->plugin_listeners, node, NULL);
	}
	cpi_unlock_context(context);
	cpi_debugf(context, "A plug-in listener was removed by %s.", cpi_context_owner(context));
}

CP_HIDDEN void cpi_deliver_event(cp_context_t *context, const cpi_plugin_event_t *event) {
	assert(event != NULL);
	assert(event->plugin_id != NULL);
	cpi_lock_context(context);
	context->env->in_event_listener_invocation++;
	list_process(context->env->plugin_listeners, (void *) event, process_event);
	context->env->in_event_listener_invocation--;
	cpi_unlock_context(context);
	if (cpi_is_logged(context, CP_LOG_INFO)) {
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

CP_C_API cp_status_t cp_register_pcollection(cp_context_t *context, const char *dir) {
	char *d = NULL;
	lnode_t *node = NULL;
	cp_status_t status = CP_OK;
	
	CHECK_NOT_NULL(context);
	CHECK_NOT_NULL(dir);
	
	cpi_lock_context(context);
	cpi_check_invocation(context, CPI_CF_ANY, __func__);
	do {
	
		// Check if directory has already been registered 
		if (list_find(context->env->plugin_dirs, dir, (int (*)(const void *, const void *)) strcmp) != NULL) {
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
		list_append(context->env->plugin_dirs, node);
		
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
	
	// Report error
	if (status == CP_ERR_RESOURCE) {
		cpi_errorf(context, _("Could not register plug-in collection directory %s due to insufficient memory."), dir);
	}

	// Report success
	if (status == CP_OK) {
		cpi_debugf(context, "Plug-in collection directory %s was registered.", dir);
	}
	
	return status;
}

CP_C_API void cp_unregister_pcollection(cp_context_t *context, const char *dir) {
	char *d;
	lnode_t *node;
	
	CHECK_NOT_NULL(context);
	CHECK_NOT_NULL(dir);
	
	cpi_lock_context(context);
	cpi_check_invocation(context, CPI_CF_ANY, __func__);
	node = list_find(context->env->plugin_dirs, dir, (int (*)(const void *, const void *)) strcmp);
	if (node != NULL) {
		d = lnode_get(node);
		list_delete(context->env->plugin_dirs, node);
		lnode_destroy(node);
		free(d);
	}
	cpi_unlock_context(context);
	cpi_debugf(context, "Plug-in collection directory %s was unregistered.", dir);
}

CP_C_API void cp_unregister_pcollections(cp_context_t *context) {
	CHECK_NOT_NULL(context);
	cpi_lock_context(context);
	cpi_check_invocation(context, CPI_CF_ANY, __func__);
	list_process(context->env->plugin_dirs, NULL, cpi_process_free_ptr);
	cpi_unlock_context(context);
	cpi_debug(context, "All plug-in collection directories were unregistered.");
}


// Startup arguments

CP_C_API void cp_set_context_args(cp_context_t *ctx, int argc, char **argv) {
	CHECK_NOT_NULL(ctx);
	CHECK_NOT_NULL(argv);
	cpi_lock_context(ctx);
	ctx->env->argc = argc;
	ctx->env->argv = argv;
	cpi_unlock_context(ctx);
}

CP_C_API int cp_get_context_args(cp_context_t *ctx, char ***argv) {
	int argc;
	
	CHECK_NOT_NULL(ctx);
	CHECK_NOT_NULL(argv);
	cpi_lock_context(ctx);
	argc = ctx->env->argc;
	*argv = ctx->env->argv;
	cpi_unlock_context(ctx);
	return argc;
}


// Checking API call invocation

CP_HIDDEN void cpi_check_invocation(cp_context_t *ctx, int funcmask, const char *func) {
	assert(ctx != NULL);
	assert(funcmask != 0);
	assert(func != NULL);
#ifdef CP_THREADS
	assert(cpi_is_mutex_locked(ctx->env->mutex));
#else
	assert(ctx->env->locked);
#endif
	if ((funcmask & CPI_CF_LOGGER)
		&&ctx->env->in_logger_invocation) {
		cpi_fatalf(_("%s was called from within a logger invocation."), func);
	}
	if ((funcmask & CPI_CF_LISTENER)
		&& ctx->env->in_event_listener_invocation) {
		cpi_fatalf(_("%s was called from within an event listener invocation."), func);
	}
	if ((funcmask & CPI_CF_START)
		&& ctx->env->in_start_func_invocation) {
		cpi_fatalf(_("%s was called from within a start function invocation."), func);
	}
	if ((funcmask & CPI_CF_STOP)
		&& ctx->env->in_stop_func_invocation) {
		cpi_fatalf(_("%s was called from within a stop function invocation."), func);
	}
	if (ctx->env->in_create_func_invocation) {
		cpi_fatalf(_("%s was called from within a create function invocation."), func);
	}
	if (ctx->env->in_destroy_func_invocation) {
		cpi_fatalf(_("%s was called from within a destroy function invocation."), func);
	}
}


// Locking 

#if defined(CP_THREADS) || !defined(NDEBUG)

CP_HIDDEN void cpi_lock_context(cp_context_t *context) {
#if defined(CP_THREADS)
	cpi_lock_mutex(context->env->mutex);
#elif !defined(NDEBUG)
	context->env->locked++;
#endif
}

CP_HIDDEN void cpi_unlock_context(cp_context_t *context) {
#if defined(CP_THREADS)
	cpi_unlock_mutex(context->env->mutex);
#elif !defined(NDEBUG)
	assert(context->env->locked > 0);
	context->env->locked--;
#endif
}

CP_HIDDEN void cpi_wait_context(cp_context_t *context) {
#if defined(CP_THREADS)
	cpi_wait_mutex(context->env->mutex);
#elif !defined(NDEBUG)
	assert(context->env->locked > 0);
	assert(0);
#endif
}

CP_HIDDEN void cpi_signal_context(cp_context_t *context) {
#if defined(CP_THREADS)
	cpi_signal_mutex(context->env->mutex);
#elif !defined(NDEBUG)
	assert(context->env->locked > 0);
#endif
}


// Debug helpers

#ifndef NDEBUG
CP_HIDDEN const char *cpi_context_owner(cp_context_t *ctx) {
	static char buffer[64];
	
	if (ctx->plugin != NULL) {
		snprintf(buffer, sizeof(buffer), "plugin %s", ctx->plugin->plugin->identifier);
	} else {
		strncpy(buffer, "the client program", sizeof(buffer));
	}
	buffer[sizeof(buffer)/sizeof(char) - 1] = '\0';
	return buffer;
}
#endif

#endif
