/*-------------------------------------------------------------------------
 * C-Pluff, a plug-in framework for C
 * Copyright 2006 Johannes Lehtinen
 *-----------------------------------------------------------------------*/

/** @file
 * Core plug-in management functions
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stddef.h>
#include "../kazlib/list.h"
#include "../kazlib/hash.h"
#include "cpluff.h"
#include "defines.h"
#include "util.h"
#include "internal.h"


/* ------------------------------------------------------------------------
 * Function definitions
 * ----------------------------------------------------------------------*/

// Plug-in control

#ifndef NDEBUG
static void assert_processed_zero(cp_context_t *context) {
	hscan_t scan;
	hnode_t *node;
	
	hash_scan_begin(&scan, context->env->plugins);
	while ((node = hash_scan_next(&scan)) != NULL) {
		cp_plugin_t *plugin = hnode_get(node);
		assert(plugin->processed == 0);
	}
}
#else
#define assert_processed_zero(c) assert(1)
#endif

static void unregister_extensions(cp_context_t *context, cp_plugin_info_t *plugin) {
	int i;
	
	for (i = 0; i < plugin->num_ext_points; i++) {
		cp_ext_point_t *ep = plugin->ext_points + i;
		hnode_t *hnode;
		
		if ((hnode = hash_lookup(context->env->ext_points, ep->global_id)) != NULL
			&& hnode_get(hnode) == ep) {
			hash_delete_free(context->env->ext_points, hnode);
		}
	}
	for (i = 0; i < plugin->num_extensions; i++) {
		cp_extension_t *e = plugin->extensions + i;
		hnode_t *hnode;
		
		if ((hnode = hash_lookup(context->env->extensions, e->ext_point_id)) != NULL) {
			list_t *el = hnode_get(hnode);
			lnode_t *lnode = list_first(el);
			
			while (lnode != NULL) {
				lnode_t *nn = list_next(el, lnode);
				if (lnode_get(lnode) == e) {
					list_delete(el, lnode);
					lnode_destroy(lnode);
					break;
				}
				lnode = nn;
			}
			if (list_isempty(el)) {
				hash_delete_free(context->env->extensions, hnode);
			}
		}
	}
}

CP_API int cp_install_plugin(cp_context_t *context, cp_plugin_info_t *plugin) {
	cp_plugin_t *rp = NULL;
	int status = CP_OK;
	cpi_plugin_event_t event;
	int i;

	CHECK_NOT_NULL(context);
	CHECK_NOT_NULL(plugin);
	
	cpi_lock_context(context);
	cpi_check_invocation(context, CPI_CF_ANY, __func__);
	do {
		
		// Check that there is no conflicting plug-in already loaded 
		if (hash_lookup(context->env->plugins, plugin->identifier) != NULL) {
			cpi_errorf(context,
				_("Plug-in %s could not be installed because a plug-in with the same identifier is already installed."), 
				plugin->identifier);
			status = CP_ERR_CONFLICT;
			break;
		}

		// Increase usage count for the plug-in descriptor
		cpi_use_info(plugin);

		// Allocate space for the plug-in state 
		if ((rp = malloc(sizeof(cp_plugin_t))) == NULL) {
			status = CP_ERR_RESOURCE;
			break;
		}
	
		// Initialize plug-in state 
		memset(rp, 0, sizeof(cp_plugin_t));
		rp->context = NULL;
		rp->plugin = plugin;
		rp->state = CP_PLUGIN_INSTALLED;
		rp->imported = NULL;
		rp->runtime_lib = NULL;
		rp->runtime_funcs = NULL;
		rp->plugin_data = NULL;
		rp->importing = list_create(LISTCOUNT_T_MAX);
		if (rp->importing == NULL) {
			status = CP_ERR_RESOURCE;
			break;
		}
		if (!hash_alloc_insert(context->env->plugins, plugin->identifier, rp)) {
			status = CP_ERR_RESOURCE;
			break;
		}
		
		// Register extension points
		for (i = 0; status == CP_OK && i < plugin->num_ext_points; i++) {
			cp_ext_point_t *ep = plugin->ext_points + i;
			hnode_t *hnode;
			
			if ((hnode = hash_lookup(context->env->ext_points, ep->global_id)) != NULL) {
				cpi_errorf(context, _("Plug-in %s could not be installed because extension point %s conflicts with an already installed extension point."), plugin->identifier, ep->global_id);
				status = CP_ERR_CONFLICT;
			} else if (!hash_alloc_insert(context->env->ext_points, ep->global_id, ep)) {
				status = CP_ERR_RESOURCE;
			}
		}
		
		// Register extensions
		for (i = 0; status == CP_OK && i < plugin->num_extensions; i++) {
			cp_extension_t *e = plugin->extensions + i;
			hnode_t *hnode;
			lnode_t *lnode;
			list_t *el;
			
			if ((hnode = hash_lookup(context->env->extensions, e->ext_point_id)) == NULL) {
				if ((el = list_create(LISTCOUNT_T_MAX)) != NULL) {
					if (!hash_alloc_insert(context->env->extensions, e->ext_point_id, el)) {
						list_destroy(el);
						status = CP_ERR_RESOURCE;
						break;
					}
				} else {
					status = CP_ERR_RESOURCE;
					break;
				}
			} else {
				el = hnode_get(hnode);
			}
			if ((lnode = lnode_create(e)) != NULL) {
				list_append(el, lnode);
			} else {
				status = CP_ERR_RESOURCE;
				break;
			}
		}

		// Break if previous loops failed
		if (status != CP_OK) {
			break;
		}
		
		// Plug-in installed 
		event.plugin_id = plugin->identifier;
		event.old_state = CP_PLUGIN_UNINSTALLED;
		event.new_state = rp->state;
		cpi_deliver_event(context, &event);

	} while (0);

	// Release resources on failure
	if (status != CP_OK) {
		if (rp != NULL) {
			if (rp->importing != NULL) {
				list_destroy(rp->importing);
			}
			free(rp);
		}
		unregister_extensions(context, plugin);
	}
	cpi_unlock_context(context);

	// Report possible resource error
	if (status == CP_ERR_RESOURCE) {
		cpi_errorf(context,
			_("Plug-in %s could not be installed due to insufficient system resources."), plugin->identifier);
	}
	return status;
}

/**
 * Unresolves the plug-in runtime information.
 * 
 * @param plugin the plug-in to unresolve
 */
static void unresolve_plugin_runtime(cp_plugin_t *plugin) {
	plugin->runtime_funcs = NULL;
	if (plugin->runtime_lib != NULL) {
		DLCLOSE(plugin->runtime_lib);
		plugin->runtime_lib = NULL;
	}	
}

/**
 * Loads and resolves the plug-in runtime library and initialization functions.
 * 
 * @param context the plug-in context
 * @param plugin the plugin
 * @return CP_OK (zero) on success or error code on failure
 */
static int resolve_plugin_runtime(cp_context_t *context, cp_plugin_t *plugin) {
	char *rlpath = NULL;
	int status = CP_OK;
	
	assert(plugin->runtime_lib == NULL);
	if (plugin->plugin->lib_path == NULL) {
		return CP_OK;
	}
	
	do {
		int ppath_len, lpath_len;
	
		// Construct a path to plug-in runtime library 
		ppath_len = strlen(plugin->plugin->plugin_path);
		lpath_len = strlen(plugin->plugin->lib_path);
		if ((rlpath = malloc(sizeof(char) *
			(ppath_len + lpath_len + strlen(CP_SHREXT) + 2))) == NULL) {
			cpi_errorf(context, _("Plug-in %s runtime could not be loaded due to insufficient memory."), plugin->plugin->identifier);
			status = CP_ERR_RESOURCE;
			break;
		}
		strcpy(rlpath, plugin->plugin->plugin_path);
		rlpath[ppath_len] = CP_FNAMESEP_CHAR;
		strcpy(rlpath + ppath_len + 1, plugin->plugin->lib_path);
		strcpy(rlpath + ppath_len + 1 + lpath_len, CP_SHREXT);
		
		// Open the plug-in runtime library 
		plugin->runtime_lib = DLOPEN(rlpath);
		if (plugin->runtime_lib == NULL) {
			cpi_errorf(context, _("Plug-in %s runtime library %s could not be opened."), plugin->plugin->identifier, plugin->plugin->lib_path);
			status = CP_ERR_RUNTIME;
			break;
		}
		
		// Resolve plug-in functions
		if (plugin->plugin->runtime_funcs_symbol != NULL) {
			plugin->runtime_funcs = (cp_plugin_runtime_t *) DLSYM(plugin->runtime_lib, plugin->plugin->runtime_funcs_symbol);
			if (plugin->runtime_funcs == NULL) {
				cpi_errorf(context, _("Plug-in %s symbol %s containing runtime function information could not be resolved."), plugin->plugin->identifier, plugin->plugin->runtime_funcs_symbol);
				status = CP_ERR_RUNTIME;
				break;
			}
			if (plugin->runtime_funcs->create == NULL
				|| plugin->runtime_funcs->destroy == NULL) {
				cpi_errorf(context, _("Plug-in %s runtime has a null constructor or destructor."), plugin->plugin->identifier);
				status = CP_ERR_RUNTIME;
				break;
			}
		}

	} while (0);
	
	// Release resources 
	free(rlpath);
	if (status != CP_OK) {
		unresolve_plugin_runtime(plugin);
	}
	
	return status;
}

/**
 * Resolves the specified plug-in import into a plug-in pointer. Does not
 * try to resolve the imported plug-in.
 * 
 * @param context the plug-in context
 * @param plugin the plug-in being resolved
 * @param import the plug-in import to resolve
 * @param ipptr filled with pointer to the resolved plug-in or NULL
 * @return CP_OK on success or error code on failure
 */
static int resolve_plugin_import(cp_context_t *context, cp_plugin_t *plugin, cp_plugin_import_t *import, cp_plugin_t **ipptr) {
	cp_plugin_t *ip = NULL;
	hnode_t *node;

	// Lookup the plug-in 
	node = hash_lookup(context->env->plugins, import->plugin_id);
	if (node != NULL) {
		ip = hnode_get(node);
	}
			
	// Check plug-in version
	if (ip != NULL
		&& import->api_version != -1
		&& (ip->plugin->api_version == -1
			|| ip->plugin->api_version < import->api_version
			|| ip->plugin->api_version - ip->plugin->api_age > import->api_version)) {
		cpi_errorf(context,
			_("Plug-in %s could not be resolved because of version incompatibility with plug-in %s."),
			plugin->plugin->identifier,
			import->plugin_id);
		*ipptr = NULL;
		return CP_ERR_DEPENDENCY;
	}
	
	// Check if missing mandatory plug-in
	if (ip == NULL && !import->optional) {
		cpi_errorf(context,
			_("Plug-in %s could not be resolved because it depends on plug-in %s which is not installed."),
			plugin->plugin->identifier,
			import->plugin_id);
		*ipptr = NULL;
		return CP_ERR_DEPENDENCY;
	}

	// Return imported plug-in
	*ipptr = ip;
	return CP_OK;
}

/**
 * Resolves the specified plug-in and its dependencies while leaving plug-ins
 * with circular dependencies in a preliminarily resolved state.
 * 
 * @param context the plug-in context
 * @param plugin the plug-in
 * @return CP_OK (zero) or CP_OK_PRELIMINARY or an error code
 */
static int resolve_plugin_prel_rec(cp_context_t *context, cp_plugin_t *plugin) {
	int status = CP_OK;
	int error_reported = 0;
	lnode_t *node = NULL;
	int i;

	// Check if already resolved
	if (plugin->state >= CP_PLUGIN_RESOLVED) {
		return CP_OK;
	}
	
	// Check for dependency loops
	if (plugin->processed) {
		return CP_OK_PRELIMINARY;
	}
	plugin->processed = 1;

	do {
	
		// Recursively resolve the imported plug-ins
		assert(plugin->imported == NULL);
		if ((plugin->imported = list_create(LISTCOUNT_T_MAX)) == NULL) {
			status = CP_ERR_RESOURCE;
			break;
		}
		for (i = 0; i < plugin->plugin->num_imports; i++) {
			cp_plugin_t *ip;
			int s;
				
			if ((node = lnode_create(NULL)) == NULL) {
				status = CP_ERR_RESOURCE;
				break;
			}
			if ((s = resolve_plugin_import(context, plugin, plugin->plugin->imports + i, &ip)) != CP_OK) {
				error_reported = 1;
				status = s;
				break;
			}
			if (ip != NULL) {
				lnode_put(node, ip);
				list_append(plugin->imported, node);
				node = NULL;
				if (!cpi_ptrset_add(ip->importing, plugin)) {
					status = CP_ERR_RESOURCE;
					break;
				} else if ((s = resolve_plugin_prel_rec(context, ip)) != CP_OK && s != CP_OK_PRELIMINARY) {
					cpi_errorf(context, _("Plug-in %s could not be resolved because it depends on plug-in %s which could not be resolved."), plugin->plugin->identifier, ip->plugin->identifier);
					error_reported = 1;
					status = s;
					break;
				}
			} else {
				lnode_destroy(node);
				node = NULL;
			}
		}
		
		// Resolve this plug-in
		assert(plugin->state == CP_PLUGIN_INSTALLED);
		if ((i = resolve_plugin_runtime(context, plugin)) != CP_OK) {
			status = i;
			error_reported = 1;
			break;
		}
		
		// Notify event listeners and update state if completely resolved
		if (status == CP_OK) {
			cpi_plugin_event_t event;
			
			plugin->processed = 0;
			event.plugin_id = plugin->plugin->identifier;
			event.old_state = plugin->state;
			event.new_state = plugin->state = CP_PLUGIN_RESOLVED;
			cpi_deliver_event(context, &event);
		}

	} while (0);

	// Clean up
	if (node != NULL) {
		lnode_destroy(node);
	}

	// Handle errors
	if (status == CP_ERR_RESOURCE && !error_reported) {
		cpi_errorf(context, _("Plug-in %s could not be resolved because of insufficient memory."), plugin->plugin->identifier);
	}
	
	return status;
}

/**
 * Recursively commits the resolving process for the specified plug-in and
 * its dependencies.
 * 
 * @param context the plug-in context
 * @param plugin the plug-in
 */
static void resolve_plugin_commit_rec(cp_context_t *context, cp_plugin_t *plugin) {
	
	// Check if already committed
	if (!plugin->processed) {
		return;
	}
	plugin->processed = 0;
	
	// Commit if only preliminarily resolved
	if (plugin->state < CP_PLUGIN_RESOLVED) {
			cpi_plugin_event_t event;
			lnode_t *node;

			// Recursively commit dependencies
			node = list_first(plugin->imported);
			while (node != NULL) {
				resolve_plugin_commit_rec(context, (cp_plugin_t *) lnode_get(node));
				node = list_next(plugin->imported, node);
			}
			
			// Notify event listeners and update state
			event.plugin_id = plugin->plugin->identifier;
			event.old_state = plugin->state;
			event.new_state = plugin->state = CP_PLUGIN_RESOLVED;
			cpi_deliver_event(context, &event);		
	}
}

/**
 * Recursively cleans up the specified plug-in and its dependencies after
 * a failed resolving attempt.
 * 
 * @param plugin the plug-in
 */
static void resolve_plugin_failed_rec(cp_plugin_t *plugin) {
	
	// Check if already cleaned up
	if (!plugin->processed) {
		return;
	}
	plugin->processed = 0;
	
	// Clean up if only preliminarily resolved
	if (plugin->state < CP_PLUGIN_RESOLVED) {
		lnode_t *node;

		// Recursively clean up depedencies
		while ((node = list_first(plugin->imported)) != NULL) {
			cp_plugin_t *ip = lnode_get(node);
			
			resolve_plugin_failed_rec(ip);
			cpi_ptrset_remove(ip->importing, plugin);
			list_delete(plugin->imported, node);
			lnode_destroy(node);
		}
		plugin->imported = NULL;
	}
}

/**
 * Resolves the specified plug-in and its dependencies.
 * 
 * @param context the plug-in context
 * @param plugin the plug-in to be resolved
 * @return CP_OK (zero) on success or an error code on failure
 */
static int resolve_plugin(cp_context_t *context, cp_plugin_t *plugin) {
	int status;
	
	if ((status = resolve_plugin_prel_rec(context, plugin)) == CP_OK || status == CP_OK_PRELIMINARY) {
		status = CP_OK;
		resolve_plugin_commit_rec(context, plugin);
	} else {
		resolve_plugin_failed_rec(plugin);
	}
	assert_processed_zero(context);
	return status;
}

/**
 * Starts the plug-in runtime of the specified plug-in. This function does
 * not consider dependencies and assumes that the plug-in is resolved but
 * not yet started.
 * 
 * @param context the plug-in context
 * @param plugin the plug-in
 * @return CP_OK (zero) on success or an error code on failure
 */
static int start_plugin_runtime(cp_context_t *context, cp_plugin_t *plugin) {
	int status = CP_OK;
	cpi_plugin_event_t event;
	lnode_t *node = NULL;

	event.plugin_id = plugin->plugin->identifier;
	do {

		// Allocate space for the list node 
		node = lnode_create(plugin);
		if (node == NULL) {
			status = CP_ERR_RESOURCE;
			break;
		}
		
		// Set up plug-in instance
		if (plugin->runtime_funcs != NULL) {
		
			// Create context
			if ((plugin->context = cpi_new_context(plugin, context->env, &status)) == NULL) {
				break;
			}
		
			// Create a plug-in instance
			context->env->in_create_func_invocation++;
			plugin->plugin_data = plugin->runtime_funcs->create(plugin->context);
			context->env->in_create_func_invocation--;
			if (plugin->plugin_data == NULL) {
				status = CP_ERR_RUNTIME;
				break;
			}
			
			// Start plug-in
			if (plugin->runtime_funcs->start != NULL) {
				int s;
			
				// About to start the plug-in 
				event.old_state = plugin->state;
				event.new_state = plugin->state = CP_PLUGIN_STARTING;
				cpi_deliver_event(context, &event);
		
				// Start the plug-in
				context->env->in_start_func_invocation++;
				s = plugin->runtime_funcs->start(plugin->plugin_data);
				context->env->in_start_func_invocation--;

				if (s != CP_OK) {
			
					// Roll back plug-in state 
					if (plugin->runtime_funcs->stop != NULL) {

						// Update state					
						event.old_state = plugin->state;
						event.new_state = plugin->state = CP_PLUGIN_STOPPING;
						cpi_deliver_event(context, &event);
					
						// Call stop function
						context->env->in_stop_func_invocation++;
						plugin->runtime_funcs->stop(plugin->plugin_data);
						context->env->in_stop_func_invocation--;
					}
				
					// Destroy plug-in object
					context->env->in_destroy_func_invocation++;
					plugin->runtime_funcs->destroy(plugin->plugin_data);
					context->env->in_destroy_func_invocation--;
			
					status = CP_ERR_RUNTIME;
					break;
				}
			}
		}
		
		// Plug-in active 
		list_append(context->env->started_plugins, node);
		event.old_state = plugin->state;
		event.new_state = plugin->state = CP_PLUGIN_ACTIVE;
		cpi_deliver_event(context, &event);
		
	} while (0);

	// Release resources and roll back plug-in state on failure
	if (status != CP_OK) {
		if (node != NULL) {
			lnode_destroy(node);
		}
		if (plugin->context != NULL) {
			cpi_free_context(plugin->context);
			plugin->context = NULL;
		}
		if (plugin->state != CP_PLUGIN_RESOLVED) {
			event.old_state = plugin->state;
			event.new_state = plugin->state = CP_PLUGIN_RESOLVED;
			cpi_deliver_event(context, &event);
		}
		plugin->plugin_data = NULL;
	}

	// Report error on failure
	switch (status) {
		case CP_ERR_RESOURCE:
			cpi_errorf(context,
				_("Plug-in %s could not be started due to insufficient memory."),
				plugin->plugin->identifier);
			break;
		case CP_ERR_RUNTIME:
			cpi_errorf(context,
				_("Plug-in %s failed to start due to plug-in runtime error."),
				plugin->plugin->identifier);
			break;
		default:
			break;
	}	
	
	return status;
}

static void warn_dependency_loop(cp_context_t *context, cp_plugin_t *plugin, list_t *importing, int dynamic) {
	char *msgbase;
	char *msg;
	int msgsize;
	lnode_t *node;
	
	// Take the message base
	if (dynamic) {
		msgbase = _("Detected a dynamic plug-in dependency loop: ");
	} else {
		msgbase = _("Detected a static plug-in dependency loop: ");
	}
	
	// Calculate the required message space
	msgsize = strlen(msgbase);
	msgsize += strlen(plugin->plugin->identifier);
	msgsize += 2;
	node = list_last(importing);
	while (node != NULL) {
		cp_plugin_t *p = lnode_get(node);
		if (p == plugin) {
			break;
		}
		msgsize += strlen(p->plugin->identifier);
		msgsize += 2;
		node = list_prev(importing, node);
	}
	msg = malloc(sizeof(char) * msgsize);
	if (msg != NULL) {
		strcpy(msg, msgbase);
		strcat(msg, plugin->plugin->identifier);
		node = list_last(importing);
		while (node != NULL) {
			cp_plugin_t *p = lnode_get(node);
			if (p == plugin) {
				break;
			}
			strcat(msg, ", ");
			strcat(msg, p->plugin->identifier);
			node = list_prev(importing, node);
		}
		strcat(msg, ".");
		cpi_warn(context, msg);
		free(msg);
	}
}

/**
 * Starts the specified plug-in and its dependencies.
 * 
 * @param context the plug-in context
 * @param plugin the plug-in
 * @param importing stack of importing plug-ins
 * @return CP_OK (zero) on success or an error code on failure
 */
static int start_plugin_rec(cp_context_t *context, cp_plugin_t *plugin, list_t *importing) {
	int status = CP_OK;
	lnode_t *node;
	
	// Check if already started or starting
	if (plugin->state == CP_PLUGIN_ACTIVE) {
		return CP_OK;
	} else if (plugin->state == CP_PLUGIN_STARTING) {
		warn_dependency_loop(context, plugin, importing, 1);
		return CP_OK;
	}
	assert(plugin->state == CP_PLUGIN_RESOLVED);
	
	// Check for dependency loops
	if (cpi_ptrset_contains(importing, plugin)) {
		warn_dependency_loop(context, plugin, importing, 0);
		return CP_OK;
	}
	if (!cpi_ptrset_add(importing, plugin)) {
		cpi_errorf(context,
			_("Plug-in %s could not be started due to insufficient memory."),
			plugin->plugin->identifier);
		return CP_ERR_RESOURCE;
	}

	// Start up dependencies
	node = list_first(plugin->imported);
	while (node != NULL) {
		cp_plugin_t *ip = lnode_get(node);
		
		if ((status = start_plugin_rec(context, ip, importing)) != CP_OK) {
			break;
		}
		node = list_next(plugin->imported, node);
	}
	cpi_ptrset_remove(importing, plugin);
	
	// Start up this plug-in
	if (status == CP_OK) {
		status = start_plugin_runtime(context, plugin);
	}

	return status;
}

CP_HIDDEN int cpi_start_plugin(cp_context_t *context, cp_plugin_t *plugin) {
	int status;
	
	if ((status = resolve_plugin(context, plugin)) == CP_OK) {
		list_t *importing = list_create(LISTCOUNT_T_MAX);
		if (importing != NULL) {
			status = start_plugin_rec(context, plugin, importing);
			assert(list_isempty(importing));
			list_destroy(importing);
		} else {
			cpi_errorf(context,
				_("Plug-in %s could not be started due to insufficient memory."),
				plugin->plugin->identifier);
			status = CP_ERR_RESOURCE;
		}
	}
	return status;
}

CP_API int cp_start_plugin(cp_context_t *context, const char *id) {
	hnode_t *node;
	int status = CP_OK;

	CHECK_NOT_NULL(context);
	CHECK_NOT_NULL(id);

	// Look up and start the plug-in 
	cpi_lock_context(context);
	cpi_check_invocation(context, CPI_CF_ANY, __func__);
	node = hash_lookup(context->env->plugins, id);
	if (node != NULL) {
		status = cpi_start_plugin(context, hnode_get(node));
	} else {
		cpi_warnf(context, _("Unknown plug-in %s could not be started."), id);
		status = CP_ERR_UNKNOWN;
	}
	cpi_unlock_context(context);

	return status;
}

/**
 * Stops the plug-in runtime of the specified plug-in. This function does
 * not consider dependencies and assumes that the plug-in is active.
 * 
 * @param context the plug-in context
 * @param plugin the plug-in
 */
static void stop_plugin_runtime(cp_context_t *context, cp_plugin_t *plugin) {
	cpi_plugin_event_t event;
	
	// Check if still in use by the main program
	if (plugin->syms_usage_count) {
		cpi_fatalf(_("Plug-in %s is being stopped while still providing symbols to the main program."), plugin->plugin->identifier);
	}

	// Wait until possible run functions have stopped
	cpi_stop_plugin_run(plugin);

	// Destroy plug-in instance
	if (plugin->runtime_funcs != NULL) {
	
		// Stop the plug-in
		event.plugin_id = plugin->plugin->identifier;
		if (plugin->runtime_funcs->stop != NULL) {

			// About to stop the plug-in 
			event.old_state = plugin->state;
			event.new_state = plugin->state = CP_PLUGIN_STOPPING;
			cpi_deliver_event(context, &event);
	
			// Invoke stop function	
			context->env->in_stop_func_invocation++;
			plugin->runtime_funcs->stop(plugin->plugin_data);
			context->env->in_stop_func_invocation--;
		}

		// Release resolved symbols
		if (plugin->context->resolved_symbols != NULL) {
			while (!hash_isempty(plugin->context->resolved_symbols)) {
				hscan_t scan;
				hnode_t *node;
				const void *ptr;
			
				hash_scan_begin(&scan, plugin->context->resolved_symbols);
				node = hash_scan_next(&scan);
				ptr = hnode_getkey(node);
				cp_release_symbol(context, ptr);
			}
			assert(hash_isempty(plugin->context->resolved_symbols));
			hash_destroy(plugin->context->resolved_symbols);
		}
		if (plugin->context->symbol_providers != NULL) {
			assert(hash_isempty(plugin->context->symbol_providers));
			hash_destroy(plugin->context->symbol_providers);
		}

		// Release defined symbols
		if (plugin->defined_symbols != NULL) {
			hscan_t scan;
			
			while (!hash_isempty(plugin->defined_symbols)) {
				hnode_t *node;
				char *n;
				
				hash_scan_begin(&scan, plugin->defined_symbols);
				node = hash_scan_next(&scan);
				n = hnode_get(node);
				hash_scan_delfree(plugin->defined_symbols, node);
				free(n);
			}
			hash_destroy(plugin->defined_symbols);
			plugin->defined_symbols = NULL;
		}
	
		// Destroy the plug-in object
		context->env->in_destroy_func_invocation++;
		plugin->runtime_funcs->destroy(plugin->plugin_data);
		context->env->in_destroy_func_invocation--;
		plugin->plugin_data = NULL;
	}
	
	// Free plug-in context
	if (plugin->context != NULL) {
		cpi_free_context(plugin->context);
		plugin->context = NULL;
	}
		
	// Plug-in stopped 
	cpi_ptrset_remove(context->env->started_plugins, plugin);
	event.old_state = plugin->state;
	event.new_state = plugin->state = CP_PLUGIN_RESOLVED;
	cpi_deliver_event(context, &event);
}

/**
 * Stops the plug-in and all plug-ins depending on it.
 * 
 * @param context the plug-in context
 * @param plugin the plug-in
 */
static void stop_plugin_rec(cp_context_t *context, cp_plugin_t *plugin) {
	lnode_t *node;
	
	// Check if already stopped
	if (plugin->state < CP_PLUGIN_ACTIVE) {
		return;
	}
	
	// Check for dependency loops
	if (plugin->processed) {
		return;
	}
	plugin->processed = 1;
	
	// Stop the depending plug-ins
	node = list_first(plugin->importing);
	while (node != NULL) {
		stop_plugin_rec(context, lnode_get(node));
		node = list_next(plugin->importing, node);
	}

	// Stop this plug-in
	assert(plugin->state == CP_PLUGIN_ACTIVE);
	stop_plugin_runtime(context, plugin);
	assert(plugin->state < CP_PLUGIN_ACTIVE);
	
	// Clear processed flag
	plugin->processed = 0;
}

static void stop_plugin(cp_context_t *context, cp_plugin_t *plugin) {
	stop_plugin_rec(context, plugin);
	assert_processed_zero(context);
}

CP_API int cp_stop_plugin(cp_context_t *context, const char *id) {
	hnode_t *node;
	cp_plugin_t *plugin;
	int status = CP_OK;

	CHECK_NOT_NULL(context);
	CHECK_NOT_NULL(id);

	// Look up and stop the plug-in 
	cpi_lock_context(context);
	cpi_check_invocation(context, CPI_CF_ANY, __func__);
	node = hash_lookup(context->env->plugins, id);
	if (node != NULL) {
		plugin = hnode_get(node);
		stop_plugin(context, plugin);
	} else {
		cpi_warnf(context, _("Unknown plug-in %s could not be stopped."), id);
		status = CP_ERR_UNKNOWN;
	}
	cpi_unlock_context(context);

	return status;
}

CP_API void cp_stop_all_plugins(cp_context_t *context) {
	lnode_t *node;
	
	CHECK_NOT_NULL(context);
	
	// Stop the active plug-ins in the reverse order they were started 
	cpi_lock_context(context);
	cpi_check_invocation(context, CPI_CF_ANY, __func__);
	while ((node = list_last(context->env->started_plugins)) != NULL) {
		stop_plugin(context, lnode_get(node));
	}
	cpi_unlock_context(context);
}

static void unresolve_plugin_rec(cp_context_t *context, cp_plugin_t *plugin) {
	lnode_t *node;
	cpi_plugin_event_t event;
	
	// Check if already unresolved
	if (plugin->state < CP_PLUGIN_RESOLVED) {
		return;
	}
	assert(plugin->state == CP_PLUGIN_RESOLVED);
	
	// Clear the list of imported plug-ins (also breaks dependency loops)
	while ((node = list_first(plugin->imported)) != NULL) {
		cp_plugin_t *ip = lnode_get(node);
		
		cpi_ptrset_remove(ip->importing, plugin);
		list_delete(plugin->imported, node);
		lnode_destroy(node);
	}
	assert(list_isempty(plugin->imported));
	list_destroy(plugin->imported);
	plugin->imported = NULL;

	// Unresolve depending plugins
	while ((node = list_first(plugin->importing)) != NULL) {
		unresolve_plugin_rec(context, lnode_get(node));
	}
	
	// Unresolve this plug-in
	unresolve_plugin_runtime(plugin);
	event.plugin_id = plugin->plugin->identifier;
	event.old_state = plugin->state;
	event.new_state = plugin->state = CP_PLUGIN_INSTALLED;
	cpi_deliver_event(context, &event);
}

/**
 * Unresolves a plug-in.
 * 
 * @param context the plug-in context
 * @param plug-in the plug-in to be unresolved
 */
static void unresolve_plugin(cp_context_t *context, cp_plugin_t *plugin) {
	stop_plugin(context, plugin);
	unresolve_plugin_rec(context, plugin);
}

static void free_plugin_import_content(cp_plugin_import_t *import) {
	assert(import != NULL);
	free(import->plugin_id);
}

static void free_ext_point_content(cp_ext_point_t *ext_point) {
	free(ext_point->name);
	free(ext_point->local_id);
	free(ext_point->global_id);
	free(ext_point->schema_path);
}

static void free_extension_content(cp_extension_t *extension) {
	free(extension->name);
	free(extension->local_id);
	free(extension->global_id);
	free(extension->ext_point_id);
}

static void free_cfg_element_content(cp_cfg_element_t *ce) {
	int i;

	assert(ce != NULL);
	free(ce->name);
	if (ce->atts != NULL) {
		free(ce->atts[0]);
		free(ce->atts);
	}
	free(ce->value);
	for (i = 0; i < ce->num_children; i++) {
		free_cfg_element_content(ce->children + i);
	}
	free(ce->children);
}

CP_HIDDEN void cpi_free_plugin(cp_plugin_info_t *plugin) {
	int i;
	
	assert(plugin != NULL);
	free(plugin->name);
	free(plugin->identifier);
	free(plugin->version);
	free(plugin->provider_name);
	free(plugin->plugin_path);
	for (i = 0; i < plugin->num_imports; i++) {
		free_plugin_import_content(plugin->imports + i);
	}
	free(plugin->imports);
	free(plugin->lib_path);
	free(plugin->runtime_funcs_symbol);
	for (i = 0; i < plugin->num_ext_points; i++) {
		free_ext_point_content(plugin->ext_points + i);
	}
	free(plugin->ext_points);
	for (i = 0; i < plugin->num_extensions; i++) {
		free_extension_content(plugin->extensions + i);
		if (plugin->extensions[i].configuration != NULL) {
			free_cfg_element_content(plugin->extensions[i].configuration);
			free(plugin->extensions[i].configuration);
		}
	}
	free(plugin->extensions);
	free(plugin);
}

/**
 * Frees any memory allocated for a registered plug-in.
 * 
 * @param plugin the plug-in to be freed
 */
static void free_registered_plugin(cp_plugin_t *plugin) {

	assert(plugin != NULL);

	// Release plug-in information
	cp_release_info(plugin->plugin);

	// Release data structures 
	if (plugin->importing != NULL) {
		assert(list_isempty(plugin->importing));
		list_destroy(plugin->importing);
	}
	assert(plugin->imported == NULL);

	free(plugin);
}

/**
 * Uninstalls a plug-in associated with the specified hash node.
 * 
 * @param context the plug-in context
 * @param node the hash node of the plug-in to be uninstalled
 */
static void uninstall_plugin(cp_context_t *context, hnode_t *node) {
	cp_plugin_t *plugin;
	cpi_plugin_event_t event;
	
	// Check if already uninstalled 
	plugin = (cp_plugin_t *) hnode_get(node);
	if (plugin->state <= CP_PLUGIN_UNINSTALLED) {
		return;
	}
	
	// Make sure the plug-in is not in resolved state 
	unresolve_plugin(context, plugin);
	assert(plugin->state == CP_PLUGIN_INSTALLED);

	// Plug-in uninstalled 
	event.plugin_id = plugin->plugin->identifier;
	event.old_state = plugin->state;
	event.new_state = plugin->state = CP_PLUGIN_UNINSTALLED;
	cpi_deliver_event(context, &event);
	
	// Unregister extension objects
	unregister_extensions(context, plugin->plugin);

	// Unregister the plug-in 
	hash_delete_free(context->env->plugins, node);

	// Free the plug-in data structures
	free_registered_plugin(plugin);
}

CP_API int cp_uninstall_plugin(cp_context_t *context, const char *id) {
	hnode_t *node;
	int status = CP_OK;

	CHECK_NOT_NULL(context);
	CHECK_NOT_NULL(id);

	// Look up and unload the plug-in 
	cpi_lock_context(context);
	cpi_check_invocation(context, CPI_CF_ANY, __func__);
	node = hash_lookup(context->env->plugins, id);
	if (node != NULL) {
		uninstall_plugin(context, node);
	} else {
		cpi_warnf(context, _("Unknown plug-in %s could not be uninstalled."), id);
		status = CP_ERR_UNKNOWN;
	}
	cpi_unlock_context(context);

	return status;
}

CP_API void cp_uninstall_all_plugins(cp_context_t *context) {
	hscan_t scan;
	hnode_t *node;
	
	CHECK_NOT_NULL(context);
	
	cpi_lock_context(context);
	cpi_check_invocation(context, CPI_CF_ANY, __func__);
	cp_stop_all_plugins(context);
	while (1) {
		hash_scan_begin(&scan, context->env->plugins);
		if ((node = hash_scan_next(&scan)) != NULL) {
			uninstall_plugin(context, node);
		} else {
			break;
		}
	}
	cpi_unlock_context(context);
}
