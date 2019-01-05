/*-------------------------------------------------------------------------
 * C-Pluff, a plug-in framework for C
 * Copyright 2007 Johannes Lehtinen
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *-----------------------------------------------------------------------*/

/** @file
 * Plug-in context implementation
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

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
 * Variables
 * ----------------------------------------------------------------------*/

/// Existing contexts
static list_t *contexts = NULL;


/* ------------------------------------------------------------------------
 * Function definitions
 * ----------------------------------------------------------------------*/

// Generic 

static void free_plugin_env(cp_plugin_env_t *env) {
	assert(env != NULL);
	
	// Free environment data
	if (env->plugin_listeners != NULL) {
		cpi_unregister_plisteners(env->plugin_listeners, NULL);
		list_destroy(env->plugin_listeners);
		env->plugin_listeners = NULL;
	}
	if (env->loggers != NULL) {
		cpi_unregister_loggers(env->loggers, NULL);
		list_destroy(env->loggers);
		env->loggers = NULL;
	}
	if (env->local_loader != NULL) {
		cp_destroy_local_ploader(env->local_loader);
		env->local_loader = NULL;
	}
	if (env->loaders_to_plugins != NULL) {
		assert(hash_isempty(env->loaders_to_plugins));
		hash_destroy(env->loaders_to_plugins);
		env->loaders_to_plugins = NULL;
	}
	if (env->infos != NULL) {
		assert(hash_isempty(env->infos));
		hash_destroy(env->infos);
		env->infos = NULL;
	}
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
	if (env->ext_points != NULL) {
		assert(hash_isempty(env->ext_points));
		hash_destroy(env->ext_points);
	}
	if (env->extensions != NULL) {
		assert(hash_isempty(env->extensions));
		hash_destroy(env->extensions);
	}
	if (env->run_funcs != NULL) {
		assert(list_isempty(env->run_funcs));
		list_destroy(env->run_funcs);
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
	if (context->resolved_symbols != NULL) {
		assert(hash_isempty(context->resolved_symbols));
		hash_destroy(context->resolved_symbols);
	}
	if (context->symbol_providers != NULL) {
		assert(hash_isempty(context->symbol_providers));
		hash_destroy(context->symbol_providers);
	}

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
		env->plugin_descriptor_name = CP_PLUGIN_DESCRIPTOR;
		env->plugin_descriptor_root_element = CP_PLUGIN_ROOT_ELEMENT;
		env->plugin_listeners = list_create(LISTCOUNT_T_MAX);
		env->loggers = list_create(LISTCOUNT_T_MAX);
		env->log_min_severity = CP_LOG_NONE;
		env->local_loader = NULL;
		env->loaders_to_plugins = hash_create(LISTCOUNT_T_MAX, cpi_comp_ptr, cpi_hashfunc_ptr);
		env->infos = hash_create(HASHCOUNT_T_MAX, cpi_comp_ptr, cpi_hashfunc_ptr);
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
			|| env->loaders_to_plugins == NULL
			|| env->infos == NULL
			|| env->plugins == NULL
			|| env->started_plugins == NULL
			|| env->ext_points == NULL
			|| env->extensions == NULL
			|| env->run_funcs == NULL) {
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

CP_C_API void cp_set_plugin_descriptor_root_element(cp_context_t *context, const char *root) {
	CHECK_NOT_NULL(context);
	CHECK_NOT_NULL(context->env);
	CHECK_NOT_NULL(root);
	context->env->plugin_descriptor_root_element = root;
}

CP_C_API void cp_set_plugin_descriptor_name(cp_context_t *context, const char *name) {
	CHECK_NOT_NULL(context);
	CHECK_NOT_NULL(context->env);
	CHECK_NOT_NULL(name);
	context->env->plugin_descriptor_name = name;
}

CP_C_API void cp_destroy_context(cp_context_t *context) {
	CHECK_NOT_NULL(context);
	if (context->plugin != NULL) {
		cpi_fatalf(_("Only the main program can destroy a plug-in context."));
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
	
	// Unregister all plug-in loaders
	cp_unregister_ploaders(context);
	
	// Unregister implicit local plug-in loader, if any
	if (context->env->local_loader != NULL) {
		cp_unregister_ploader(context, context->env->local_loader);
	}

	// Release remaining information objects
	cpi_release_infos(context);
	
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


// Plug-in directories

/*
 * Initializes the implicit local plug-in loader of the specified context.
 * Does nothing if the loader has already been initialized.
 */
static cp_status_t init_local_ploader(cp_context_t *context) {
	cp_status_t status = CP_OK;

	assert(cpi_is_context_locked(context));
	
	// Create new local plug-in loader, if one does not exist
	if (context->env->local_loader == NULL) {
		context->env->local_loader = cp_create_local_ploader(&status);
		status = cp_register_ploader(context, context->env->local_loader);
	}
	
	return status;
}

CP_C_API cp_status_t cp_register_pcollection(cp_context_t *context, const char *dir) {
	cp_status_t status = CP_OK;
	
	CHECK_NOT_NULL(context);
	CHECK_NOT_NULL(dir);
	
	cpi_lock_context(context);
	cpi_check_invocation(context, CPI_CF_ANY, __func__);
	do {
	
		// Initialize plug-in loader
		if ((status = init_local_ploader(context)) != CP_OK) {
			break;
		}
		
		// Register directory
		status = cp_lpl_register_dir(context->env->local_loader, dir);
	
	} while (0);

	// Report error or success
	if (status != CP_OK) {
		cpi_errorf(context, N_("The plug-in collection in path %s could not be registered due to insufficient memory."), dir);
	} else {
		cpi_debugf(context, N_("The plug-in collection in path %s was registered."), dir);
	}
	cpi_unlock_context(context);

	return status;
}

CP_C_API void cp_unregister_pcollection(cp_context_t *context, const char *dir) {
	CHECK_NOT_NULL(context);
	CHECK_NOT_NULL(dir);
	
	cpi_lock_context(context);
	cpi_check_invocation(context, CPI_CF_ANY, __func__);
	if (context->env->local_loader != NULL) {
		cp_lpl_unregister_dir(context->env->local_loader, dir);
	}
	cpi_debugf(context, N_("The plug-in collection in path %s was unregistered."), dir);
	cpi_unlock_context(context);
}

CP_C_API void cp_unregister_pcollections(cp_context_t *context) {
	CHECK_NOT_NULL(context);
	cpi_lock_context(context);
	cpi_check_invocation(context, CPI_CF_ANY, __func__);
	if (context->env->local_loader != NULL) {
		cp_lpl_unregister_dirs(context->env->local_loader);
	}
	cpi_debug(context, N_("All plug-in collections were unregistered."));
	cpi_unlock_context(context);
}


// Plug-in loaders

CP_C_API cp_status_t cp_register_ploader(cp_context_t *ctx, cp_plugin_loader_t *loader) {
	cp_status_t status = CP_OK;
	hash_t *loader_plugins = NULL;
	
	CHECK_NOT_NULL(ctx);
	CHECK_NOT_NULL(loader);
	
	cpi_lock_context(ctx);
	cpi_check_invocation(ctx, CPI_CF_ANY, __func__);
	do {
		if ((loader_plugins = hash_create(HASHCOUNT_T_MAX, (int (*)(const void *, const void *)) strcmp, NULL)) == NULL) {
			status = CP_ERR_RESOURCE;
			break;
		}
		if (!hash_alloc_insert(ctx->env->loaders_to_plugins, loader, loader_plugins)) {
			status = CP_ERR_RESOURCE;
			break;
		}
	} while (0);
	
	// Report error or success
	if (status != CP_OK) {
		cpi_errorf(ctx, N_("The plug-in loader %p could not be registered due to insufficient memory."), (void *) loader);
	} else {
		cpi_debugf(ctx, N_("The plug-in loader %p was registered."), (void *) loader);
	}
	cpi_unlock_context(ctx);
	
	// Release resources
	if (status != CP_OK) {
		if (loader_plugins != NULL) {
			assert(hash_isempty(loader_plugins));
			hash_destroy(loader_plugins);
		}
	}
	
	return status;
}

CP_C_API void cp_unregister_ploader(cp_context_t *ctx, cp_plugin_loader_t *loader) {
	hnode_t *hnode;
	
	CHECK_NOT_NULL(ctx);
	CHECK_NOT_NULL(loader);

	cpi_lock_context(ctx);
	cpi_check_invocation(ctx, CPI_CF_ANY, __func__);
	hnode = hash_lookup(ctx->env->loaders_to_plugins, loader);
	if (hnode != NULL) {
		hash_t *loader_plugins = hnode_get(hnode);

		// Uninstall all plug-ins loaded by the loader
		while (!hash_isempty(loader_plugins)) {
			hscan_t hscan;
			hnode_t *hnode2;
			cp_status_t status;
	
			hash_scan_begin(&hscan, loader_plugins);
			hnode2 = hash_scan_next(&hscan);
			assert(hnode2 != NULL);
			status = cp_uninstall_plugin(ctx, hnode_getkey(hnode2));
			assert(status == CP_OK);
		}
		hash_delete_free(ctx->env->loaders_to_plugins, hnode);
		assert(hash_isempty(loader_plugins));
		hash_destroy(loader_plugins);
		cpi_debugf(ctx, N_("The plug-in loader %p was unregistered."), (void *) loader);
	}
	cpi_unlock_context(ctx);
}

CP_C_API void cp_unregister_ploaders(cp_context_t *ctx) {
	int found;

	CHECK_NOT_NULL(ctx);
	
	cpi_lock_context(ctx);
	cpi_check_invocation(ctx, CPI_CF_ANY, __func__);
	do {
		hscan_t hscan;
		hnode_t *hnode;
		
		hash_scan_begin(&hscan, ctx->env->loaders_to_plugins);
		do {
			hnode = hash_scan_next(&hscan);
		} while (hnode != NULL && hnode_getkey(hnode) == ctx->env->local_loader);
		if (hnode != NULL) {
			cp_plugin_loader_t *loader = (cp_plugin_loader_t *) hnode_getkey(hnode);
			cp_unregister_ploader(ctx, loader);
			found = 1;
		} else {
			found = 0;
		}
	} while (found);
	cpi_unlock_context(ctx);
	
}


// Startup arguments

CP_C_API void cp_set_context_args(cp_context_t *ctx, char **argv) {
	int argc;
	
	CHECK_NOT_NULL(ctx);
	CHECK_NOT_NULL(argv);
	for (argc = 0; argv[argc] != NULL; argc++);
	if (argc < 1) {
		cpi_fatalf(_("At least one startup argument must be given in call to function %s."), __func__);
	}
	cpi_lock_context(ctx);
	ctx->env->argc = argc;
	ctx->env->argv = argv;
	cpi_unlock_context(ctx);
}

CP_C_API char **cp_get_context_args(cp_context_t *ctx, int *argc) {
	char **argv;
	
	CHECK_NOT_NULL(ctx);
	cpi_lock_context(ctx);
	if (argc != NULL) {
		*argc = ctx->env->argc;
	}
	argv = ctx->env->argv;
	cpi_unlock_context(ctx);
	return argv;
}


// Checking API call invocation

CP_HIDDEN void cpi_check_invocation(cp_context_t *ctx, int funcmask, const char *func) {
	assert(ctx != NULL);
	assert(funcmask != 0);
	assert(func != NULL);
	assert(cpi_is_context_locked(ctx));
	if ((funcmask & CPI_CF_LOGGER)
		&&ctx->env->in_logger_invocation) {
		cpi_fatalf(_("Function %s was called from within a logger invocation."), func);
	}
	if ((funcmask & CPI_CF_LISTENER)
		&& ctx->env->in_event_listener_invocation) {
		cpi_fatalf(_("Function %s was called from within an event listener invocation."), func);
	}
	if ((funcmask & CPI_CF_START)
		&& ctx->env->in_start_func_invocation) {
		cpi_fatalf(_("Function %s was called from within a plug-in start function invocation."), func);
	}
	if ((funcmask & CPI_CF_STOP)
		&& ctx->env->in_stop_func_invocation) {
		cpi_fatalf(_("Function %s was called from within a plug-in stop function invocation."), func);
	}
	if (ctx->env->in_create_func_invocation) {
		cpi_fatalf(_("Function %s was called from within a plug-in create function invocation."), func);
	}
	if (ctx->env->in_destroy_func_invocation) {
		cpi_fatalf(_("Function %s was called from within a plug-in destroy function invocation."), func);
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

CP_HIDDEN char *cpi_context_owner(cp_context_t *ctx, char *name, size_t size) {
	if (ctx->plugin != NULL) {
		/* TRANSLATORS: The context owner (when it is a plug-in) used in some strings.
		   Search for "context owner" to find these strings. */
		snprintf(name, size, _("Plug-in %s"), ctx->plugin->plugin->identifier);
	} else {
		/* TRANSLATORS: The context owner (when it is the main program) used in some strings.
		   Search for "context owner" to find these strings. */
		strncpy(name, _("The main program"), size);
	}
	assert(size >= 4);
	strcpy(name + size - 4, "...");
	return name;
}

#endif
