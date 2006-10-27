/*-------------------------------------------------------------------------
 * C-Pluff, a plug-in framework for C
 * Copyright 2006 Johannes Lehtinen
 *-----------------------------------------------------------------------*/

/*
 * Plug-in management functionality
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stddef.h>
#ifdef HAVE_LIBDL
#include <dlfcn.h>
#endif
#ifdef HAVE_LIBLTDL
#include <ltdl.h>
#endif
#include "../kazlib/list.h"
#include "../kazlib/hash.h"
#include "cpluff.h"
#include "defines.h"
#include "util.h"
#include "internal.h"


/* ------------------------------------------------------------------------
 * Defines
 * ----------------------------------------------------------------------*/

#if defined(HAVE_LIBDL) && defined(HAVE_LIBLTDL)
#error Can not use both Posix dlopen support and GNU Libtool libltdl support.
#endif

#ifdef HAVE_LIBDL
#define DLOPEN(name) dlopen((name), RTLD_LAZY | RTLD_GLOBAL)
#define DLSYM(handle, symbol) dlsym((handle), (symbol))
#define DLCLOSE(handle) dlclose(handle)
#define DLHANDLE void *
#endif

#ifdef HAVE_LIBLTDL
#define DLOPEN(name) lt_dlopen(name)
#define DLSYM(handle, symbol) lt_dlsym((handle), (symbol))
#define DLCLOSE(handle) lt_dlclose(handle)
#define DLHANDLE lt_dlhandle
#endif

/* ------------------------------------------------------------------------
 * Data types
 * ----------------------------------------------------------------------*/

// Forward type definitions 
typedef struct registered_plugin_t registered_plugin_t;

/// Stores the state of a registered plug-in. 
struct registered_plugin_t {
	
	/// Plug-in information 
	cp_plugin_info_t *plugin;
	
	/// The current state of the plug-in 
	cp_plugin_state_t state;
	
	/// The set of imported plug-ins, or NULL if not resolved 
	list_t *imported;
	
	/// The set of plug-ins importing this plug-in 
	list_t *importing;
	
	/// The runtime library handle, or NULL if not resolved 
	DLHANDLE runtime_lib;
	
	/// The start function, or NULL if none or not resolved 
	cp_start_t start_func;
	
	/// The stop function, or NULL if none or not resolved 
	cp_stop_t stop_func;
	
	/// The phase of ongoing resolve/unresolve operation or 0 if none
	int phase;

};


/* ------------------------------------------------------------------------
 * Function definitions
 * ----------------------------------------------------------------------*/

// Plug-in control

int CP_API cp_install_plugin(cp_context_t *context, cp_plugin_info_t *plugin) {
	registered_plugin_t *rp;
	int status = CP_OK;
	cp_plugin_event_t event;

	assert(plugin != NULL);
	
	cpi_check_invocation(context, __func__);
	cpi_lock_context(context);
	do {

		// Check that there is no conflicting plug-in already loaded 
		if (hash_lookup(context->plugins, plugin->identifier) != NULL) {
			status = CP_ERR_CONFLICT;
			break;
		}

		// Increase usage count for the plug-in descriptor
		cpi_use_resource(plugin);

		// Allocate space for the plug-in state 
		if ((rp = malloc(sizeof(registered_plugin_t))) == NULL) {
			status = CP_ERR_RESOURCE;
			break;
		}
	
		// Initialize plug-in state 
		memset(rp, 0, sizeof(registered_plugin_t));
		rp->plugin = plugin;
		rp->state = CP_PLUGIN_INSTALLED;
		rp->imported = NULL;
		rp->runtime_lib = NULL;
		rp->start_func = NULL;
		rp->stop_func = NULL;
		rp->importing = list_create(LISTCOUNT_T_MAX);
		if (rp->importing == NULL) {
			free(rp);
			status = CP_ERR_RESOURCE;
			break;
		}
		if (!hash_alloc_insert(context->plugins, plugin->identifier, rp)) {
			list_destroy(rp->importing);
			free(rp);
			status = CP_ERR_RESOURCE;
			break;
		}
		
		// Plug-in installed 
		event.plugin_id = plugin->identifier;
		event.old_state = CP_PLUGIN_UNINSTALLED;
		event.new_state = rp->state;
		cpi_deliver_event(context, &event);

	} while (0);
	cpi_unlock_context(context);

	switch (status) {
		case CP_OK:
			break;
		case CP_ERR_CONFLICT:
			cpi_errorf(context,
				_("Plug-in %s could not be installed because a plug-in with the same identifier is already installed."), 
				plugin->identifier);
			break;
		case CP_ERR_RESOURCE:
			cpi_errorf(context,
				_("Plug-in %s could not be installed due to insufficient system resources."), plugin->identifier);
			break;
		default:
			cpi_errorf(context,
				_("Could not install plug-in %s."), plugin->identifier);
			break;
	}
	return status;
}

/**
 * Unresolves the plug-in runtime information.
 * 
 * @param plugin the plug-in to unresolve
 */
static void unresolve_plugin_runtime(registered_plugin_t *plugin) {
	plugin->start_func = NULL;
	plugin->stop_func = NULL;
	if (plugin->runtime_lib != NULL) {
		DLCLOSE(plugin->runtime_lib);
		plugin->runtime_lib = NULL;
	}	
}

/**
 * Unresolves a preliminarily resolved plug-in.
 * 
 * @param context the plug-in context
 * @param plugin the plug-in to be unresolved
 * @param failed the failed plug-in
 * @param preliminary list of preliminary resolved plug-ins
 */
static void unresolve_preliminary_plugin(cp_context_t *context,
registered_plugin_t *plugin, registered_plugin_t *failed,
list_t *preliminary) {
	lnode_t *node;

	assert(cpi_ptrset_contains(preliminary, plugin));

	// Error message 
	cpi_errorf(context,
		_("Plug-in %s could not be resolved because it depends on plug-in %s which could not be resolved."),
		plugin->plugin->identifier,
		failed->plugin->identifier);
	
	// Remove references to imported plug-ins 
	assert(plugin->imported != NULL);
	while ((node = list_first(plugin->imported)) != NULL) {
		registered_plugin_t *ip = lnode_get(node);
		cpi_ptrset_remove(ip->importing, plugin);
		list_delete(plugin->imported, node);
		lnode_destroy(node);
	}
	list_destroy(plugin->imported);
	plugin->imported = NULL;
		
	// Unresolve plug-ins which import this plug-in 
	while ((node = list_first(plugin->importing)) != NULL) {
		registered_plugin_t *ip = lnode_get(node);
		unresolve_preliminary_plugin(context, ip, plugin, preliminary);
	}
	
	// Unresolve the plug-in runtime 
	unresolve_plugin_runtime(plugin);
}

/**
 * Resolves the plug-in runtime library.
 * 
 * @param context the plug-in context
 * @param plugin the plugin
 */
static int resolve_plugin_runtime(cp_context_t *context, registered_plugin_t *plugin) {
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
			cpi_errorf(context, _("Plug-in %s runtime could not be loaded due to insufficient system resources."), plugin->plugin->identifier);
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
		
		// Resolve start and stop functions 
		if (plugin->plugin->start_func_name != NULL) {
			plugin->start_func = (cp_start_t) DLSYM(plugin->runtime_lib, plugin->plugin->start_func_name);
			if (plugin->start_func == NULL) {
				cpi_errorf(context, _("Plug-in %s start function %s could not be resolved."), plugin->plugin->identifier, plugin->plugin->start_func_name);
				status = CP_ERR_RUNTIME;
				break;
			}
		}
		if (plugin->plugin->stop_func_name != NULL) {
			plugin->stop_func = (cp_stop_t) DLSYM(plugin->runtime_lib, plugin->plugin->stop_func_name);
			if (plugin->stop_func == NULL) {
				cpi_errorf(context, _("Plug-in %s stop function %s could not be resolved."), plugin->plugin->identifier, plugin->plugin->stop_func_name);
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
 * Recursively resolves a plug-in and its dependencies. Plug-ins with
 * circular dependencies are only preliminary resolved (state is locked
 * but not updated).
 * 
 * @param context the plug-in context
 * @param plugin the plug-in to be resolved
 * @param seen list of seen plug-ins
 * @param preliminary list of preliminarily resolved plug-ins
 * @return CP_OK (0) or CP_OK_PRELIMINARY (1) on success, an error code on failure
 */
static int resolve_plugin_rec
(cp_context_t *context, registered_plugin_t *plugin, list_t *seen,
list_t *preliminary) {
	int i;
	int status = CP_OK;
	int error_reported = 0;
	
	// Check if already resolved 
	if (plugin->state >= CP_PLUGIN_RESOLVED) {
		return CP_OK;
	}
	assert(plugin->state == CP_PLUGIN_INSTALLED);
	
	// Check if already seen 
	if (cpi_ptrset_contains(seen, plugin)) {
		return CP_OK_PRELIMINARY;
	}
	
	do {
	
		// Put the plug-in into seen list
		if (!cpi_ptrset_add(seen, plugin)) {
			status = CP_ERR_RESOURCE;
			break;
		}

		// Allocate space for the import information 
		plugin->imported = list_create(LISTCOUNT_T_MAX);
		if (plugin->imported == NULL) {
			status = CP_ERR_RESOURCE;
			break;
		}
	
		// Check that all imported plug-ins are present and resolved 
		for (i = 0;	i < plugin->plugin->num_imports; i++) {
			registered_plugin_t *ip;
			hnode_t *node;
			int rc = CP_OK;
			int vermismatch = 0;
		
			// Lookup the plug-in 
			node = hash_lookup(context->plugins, plugin->plugin->imports[i].plugin_id);
			if (node != NULL) {
				ip = hnode_get(node);
			} else {
				ip = NULL;
			}
			
			// Check plug-in version 
			if (ip != NULL && plugin->plugin->imports[i].version != NULL) {
				const char *iv = ip->plugin->version;
				const char *rv = plugin->plugin->imports[i].version;
				
				switch (plugin->plugin->imports[i].match) {
					case CP_MATCH_NONE:
						break;
					case CP_MATCH_PERFECT:
						vermismatch = (cpi_version_cmp(iv, rv, 4) != 0);
						break;
					case CP_MATCH_EQUIVALENT:
						vermismatch = (cpi_version_cmp(iv, rv, 2) != 0
							|| cpi_version_cmp(iv, rv, 4) < 0);
						break;
					case CP_MATCH_COMPATIBLE:
						vermismatch = (cpi_version_cmp(iv, rv, 1) != 0
							|| cpi_version_cmp(iv, rv, 4) < 0);
						break;
					case CP_MATCH_GREATEROREQUAL:
						vermismatch = (cpi_version_cmp(iv, rv, 4) < 0);
						break;
					default:
						cpi_fatalf(_("Encountered unimplemented version match type."));
						break;
				}
			}
		
			// Check for version mismatch 
			if (vermismatch) {
				cpi_errorf(context,
					_("Plug-in %s could not be resolved because of version incompatibility with plug-in %s."),
					plugin->plugin->identifier,
					plugin->plugin->imports[i].plugin_id);
				error_reported = 1;
				status = CP_ERR_DEPENDENCY;
				break;
			}
		
			// Try to resolve the plugin 
			if (ip != NULL) {
				rc = resolve_plugin_rec(context, ip, seen, preliminary);
				
				// If import was succesful, register the dependency 
				if (rc == CP_OK || rc == CP_OK_PRELIMINARY) {
					if (!cpi_ptrset_add(plugin->imported, ip)
						|| !cpi_ptrset_add(ip->importing, plugin)) {
						status = CP_ERR_RESOURCE;
						break;
					}
					if (rc == CP_OK_PRELIMINARY) {
						status = CP_OK_PRELIMINARY;
					}
				}				
				
			}

			// Handle failure if import was mandatory 
			if (!plugin->plugin->imports[i].optional) {
				if (ip == NULL) {
					cpi_errorf(context,
						_("Plug-in %s could not be resolved because it depends on plug-in %s which is not installed."),
						plugin->plugin->identifier,
						plugin->plugin->imports[i].plugin_id);
					error_reported = 1;
					status = CP_ERR_DEPENDENCY;
					break;
				} else if (rc != CP_OK && rc != CP_OK_PRELIMINARY) {
					cpi_errorf(context,
						_("Plug-in %s could not be resolved because it depends on plug-in %s which could not be resolved."),
						plugin->plugin->identifier,
						plugin->plugin->imports[i].plugin_id);
					error_reported = 1;
					status = rc;
					break;
				}
			}
		
		}
		if (status != CP_OK && status != CP_OK_PRELIMINARY) {
			break;
		}

		// Resolve the plug-in runtime 
		if ((i = resolve_plugin_runtime(context, plugin)) != CP_OK) {
			error_reported = 1;
			status = i;
			break;
		}
		
		// Update state and inform listeners on definite success 
		if (status == CP_OK) {
			cp_plugin_event_t event;
		
			event.plugin_id = plugin->plugin->identifier;
			event.old_state = plugin->state;
			event.new_state = plugin->state = CP_PLUGIN_RESOLVED;
			cpi_deliver_event(context, &event);
		}
	
		// Postpone updates on preliminary success 
		else {
			if (!cpi_ptrset_add(preliminary, plugin)) {
				status = CP_ERR_RESOURCE;
				break;
			}
		}
		
	} while (0);
	
	// Clean up on failure 
	if (status != CP_OK && status != CP_OK_PRELIMINARY) {
		lnode_t *node;

		/* 
		 * Unresolve plug-ins which import this plug-in (if there were
		 * circular dependencies while resolving this plug-in)
		 */
		while ((node = list_first(plugin->importing)) != NULL) {
			registered_plugin_t *ip = lnode_get(node);
			unresolve_preliminary_plugin(context, ip, plugin, preliminary);
			assert(!cpi_ptrset_contains(plugin->importing, ip));
		}

		// Unresolve the plug-in runtime 
		unresolve_plugin_runtime(plugin);
		
		// Remove references to imported plug-ins 
		while ((node = list_first(plugin->imported)) != NULL) {
			registered_plugin_t *ip = lnode_get(node);
			cpi_ptrset_remove(ip->importing, plugin);
			list_delete(plugin->imported, node);
			lnode_destroy(node);
		}
		list_destroy(plugin->imported);
		plugin->imported = NULL;

		// Remove plug-in from the seen list 
		cpi_ptrset_remove(seen, plugin);
		
		// Report errors 
		if (!error_reported) {
			cpi_errorf(context, _("Plug-in %s could not be resolved due to insufficient system resources."), plugin->plugin->identifier);
		}
	
	}
	
	return status;
}

/**
 * Resolves a plug-in. Resolves all dependencies of a plug-in and loads
 * the plug-in runtime library.
 * 
 * @param context the plug-in context
 * @param plugin the plug-in to be resolved
 * @return CP_OK (0) on success, an error code on failure
 */
static int resolve_plugin(cp_context_t *context, registered_plugin_t *plugin) {
	list_t *seen = NULL, *preliminary = NULL;
	int status = CP_OK;
	
	assert(context != NULL);
	assert(plugin != NULL);
	
	// Check if already resolved 
	if (plugin->state >= CP_PLUGIN_RESOLVED) {
		return CP_OK;
	}

	do {
		lnode_t *node;

		// Create plug-in collections 
		seen = list_create(LISTCOUNT_T_MAX);
		preliminary = list_create(LISTCOUNT_T_MAX);
		if (seen == NULL || preliminary == NULL) {
			status = CP_ERR_RESOURCE;
			cpi_errorf(context, _("Plug-in %s could not be resolved due to insufficient system resources."), plugin->plugin->identifier);
			break;
		}

		// Resolve this plug-in recursively 
		status = resolve_plugin_rec(context, plugin, seen, preliminary);
		if (status == CP_OK_PRELIMINARY) {
			status = CP_OK;
		}
	
		// Plug-ins in the preliminary list are now resolved 
		if (status == CP_OK) {
			while ((node = list_first(preliminary)) != NULL) {
				registered_plugin_t *rp;
				cp_plugin_event_t event;
		
				rp = lnode_get(node);
				event.plugin_id = rp->plugin->identifier;
				event.old_state = rp->state;
				event.new_state = rp->state = CP_PLUGIN_RESOLVED;
				cpi_deliver_event(context, &event);
				list_delete(preliminary, node);
				lnode_destroy(node);
			}
		}
		
		// On error unresolve plug-in runtimes of preliminary resolved plug-ins 
		else {
			while ((node = list_last(preliminary)) != NULL) {
				registered_plugin_t *rp;
				
				rp = lnode_get(node);
				unresolve_plugin_runtime(rp);
				list_delete(preliminary, node);
				lnode_destroy(node);
			}
		}
		
		// Destroy the list of seen plug-ins
		while ((node = list_first(seen)) != NULL) {
			registered_plugin_t *rp;
			
			rp = lnode_get(node);
			list_delete(seen, node);
			lnode_destroy(node);
		}

	} while (0);

	// Release data structures 
	if (seen != NULL) {
		assert(list_isempty(seen));
		list_destroy(seen);
	}
	if (preliminary != NULL) {
		list_destroy_nodes(preliminary);
		list_destroy(preliminary);
	}

	return status;
}

/**
 * Starts a plug-in.
 * 
 * @param context the plug-in context
 * @param plugin the plug-in to be started
 * @return CP_OK (0) on success, an error code on failure
 */
static int start_plugin(cp_context_t *context, registered_plugin_t *plugin) {
	int status;
	cp_plugin_event_t event;
	lnode_t *node;
	
	// Check if already active 
	if (plugin->state >= CP_PLUGIN_ACTIVE) {
		return CP_OK;
	}

	// Make sure the plug-in is resolved 
	status = resolve_plugin(context, plugin);
	if (status != CP_OK) {
		return status;
	}
	assert(plugin->state == CP_PLUGIN_RESOLVED);
	
	do {
	
		// About to start the plug-in 
		event.plugin_id = plugin->plugin->identifier;
		event.old_state = plugin->state;
		event.new_state = plugin->state = CP_PLUGIN_STARTING;
		cpi_deliver_event(context, &event);
		
		// Allocate space for the list node 
		node = lnode_create(plugin);
		if (node == NULL) {
			status = CP_ERR_RESOURCE;
			break;
		}
		
		// Start the plug-in 
		if (plugin->start_func != NULL) {
			int s;
			
			cpi_inc_start_invocation(context);
			s = plugin->start_func(context);
			cpi_dec_start_invocation(context);
			if (!s) {
			
				// Roll back plug-in state 
				event.old_state = plugin->state;
				event.new_state = plugin->state = CP_PLUGIN_STOPPING;
				cpi_deliver_event(context, &event);
				if (plugin->stop_func != NULL) {
					cpi_inc_stop_invocation(context);
					plugin->stop_func();
					cpi_dec_stop_invocation(context);
				}
				event.old_state = plugin->state;
				event.new_state = plugin->state = CP_PLUGIN_RESOLVED;
				cpi_deliver_event(context, &event);
			
				// Release the list node 
				lnode_destroy(node);

				status = CP_ERR_RUNTIME;
				break;
			}
		}
		
		// Plug-in started 
		list_append(context->started_plugins, node);
		event.old_state = plugin->state;
		event.new_state = plugin->state = CP_PLUGIN_ACTIVE;
		cpi_deliver_event(context, &event);
		
	} while (0);

	switch (status) {
		case CP_OK:
			break;
		case CP_ERR_RESOURCE:
			cpi_errorf(context,
				_("Plug-in %s could not be started due to insufficient system resources."),
				plugin->plugin->identifier);
			break;
		case CP_ERR_RUNTIME:
			cpi_errorf(context,
				_("Plug-in %s failed to start due to runtime error."),
				plugin->plugin->identifier);
			break;
		default:
			cpi_errorf(context,
				_("Plug-in %s could not be started."), plugin->plugin->identifier);
			break;
	}	
	
	return status;
}

int CP_API cp_start_plugin(cp_context_t *context, const char *id) {
	hnode_t *node;
	registered_plugin_t *plugin;
	int status = CP_OK;

	assert(id != NULL);

	// Look up and start the plug-in 
	cpi_check_invocation(context, __func__);
	cpi_lock_context(context);
	node = hash_lookup(context->plugins, id);
	if (node != NULL) {
		plugin = hnode_get(node);
		status = start_plugin(context, plugin);
	} else {
		status = CP_ERR_UNKNOWN;
	}
	cpi_unlock_context(context);

	return status;
}

/**
 * Stops a plug-in.
 * 
 * @param context the plug-in context
 * @param plugin the plug-in to be stopped
 */
static void stop_plugin(cp_context_t *context, registered_plugin_t *plugin) {
	cp_plugin_event_t event;
	
	// Check if already stopped 
	if (plugin->state < CP_PLUGIN_ACTIVE) {
		return;
	}
	assert(plugin->state == CP_PLUGIN_ACTIVE);
			
	// About to stop the plug-in 
	event.plugin_id = plugin->plugin->identifier;
	event.old_state = plugin->state;
	event.new_state = plugin->state = CP_PLUGIN_STOPPING;
	cpi_deliver_event(context, &event);
		
	// Stop the plug-in 
	if (plugin->stop_func != NULL) {
		cpi_inc_stop_invocation(context);
		plugin->stop_func();
		cpi_dec_stop_invocation(context);
	}
		
	// Plug-in stopped 
	cpi_ptrset_remove(context->started_plugins, plugin);
	event.old_state = plugin->state;
	event.new_state = plugin->state = CP_PLUGIN_RESOLVED;
	cpi_deliver_event(context, &event);
}

int CP_API cp_stop_plugin(cp_context_t *context, const char *id) {
	hnode_t *node;
	registered_plugin_t *plugin;
	int status = CP_OK;

	assert(id != NULL);

	// Look up and stop the plug-in 
	cpi_check_invocation(context, __func__);
	cpi_lock_context(context);
	node = hash_lookup(context->plugins, id);
	if (node != NULL) {
		plugin = hnode_get(node);
		stop_plugin(context, plugin);
	} else {
		status = CP_ERR_UNKNOWN;
	}
	cpi_unlock_context(context);

	return status;
}

void CP_API cp_stop_all_plugins(cp_context_t *context) {
	lnode_t *node;
	
	// Stop the active plug-ins in the reverse order they were started 
	cpi_check_invocation(context, __func__);
	cpi_lock_context(context);
	while ((node = list_last(context->started_plugins)) != NULL) {
		stop_plugin(context, lnode_get(node));
	}
	cpi_unlock_context(context);
}

/**
 * Recursively unresolves the specified plug-in and the plug-ins depending
 * on it.
 *
 * @param context the plug-in context
 * @param plug-in the plug-in to be unresolved
 * @param phase the current phase of operation (2 for stopping,
 * 			1 for unresolve runtime and inform listeners and 0 for clean up)
 */
static void unresolve_plugin_rec(cp_context_t *context, registered_plugin_t *plugin, int phase) {
	lnode_t *node;
	cp_plugin_event_t event;

	// Check if already fully unresolved or phase already ongoing
	if (plugin->imported == NULL || plugin->phase == phase) {
		return;
	}
	
	// Start the specified phase for this plug-in
	plugin->phase = phase;

	// Recursively process dependent plug-ins
	node = list_last(plugin->importing);
	while (node != NULL) {
		unresolve_plugin_rec(context, lnode_get(node), phase);
		if (phase) {
			node = list_prev(plugin->importing, node);
		} else {
			node = list_last(plugin->importing);
		}
	}

	// Process this plug-in
	switch (phase) {
		
		case 2: // stop
			stop_plugin(context, plugin);
			assert(plugin->state == CP_PLUGIN_RESOLVED);
			break;
			
		case 1: // unresolve runtime and inform listeners
			unresolve_plugin_runtime(plugin);
			event.plugin_id = plugin->plugin->identifier;
			event.old_state = plugin->state;
			event.new_state = plugin->state = CP_PLUGIN_INSTALLED;
			cpi_deliver_event(context, &event);
			break;
			
		case 0: // clean up
			while ((node = list_first(plugin->imported)) != NULL) {
				registered_plugin_t *ip = lnode_get(node);
				cpi_ptrset_remove(ip->importing, plugin);
				list_delete(plugin->imported, node);
				lnode_destroy(node);
			}
			list_destroy(plugin->imported);
			plugin->imported = NULL;
			break;
	}
}

/**
 * Unresolves a plug-in.
 * 
 * @param context the plug-in context
 * @param plug-in the plug-in to be unresolved
 */
static void unresolve_plugin(cp_context_t *context, registered_plugin_t *plugin) {
	unresolve_plugin_rec(context, plugin, 2);
	unresolve_plugin_rec(context, plugin, 1);
	unresolve_plugin_rec(context, plugin, 0);
}

static void free_plugin_import_content(cp_plugin_import_t *import) {
	assert(import != NULL);
	free(import->plugin_id);
	free(import->version);
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

void CP_LOCAL cpi_free_plugin(cp_plugin_info_t *plugin) {
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
	free(plugin->start_func_name);
	free(plugin->stop_func_name);
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
static void free_registered_plugin(registered_plugin_t *plugin) {

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
	registered_plugin_t *plugin;
	cp_plugin_event_t event;
	
	// Check if already uninstalled 
	plugin = (registered_plugin_t *) hnode_get(node);
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
	
	// Unregister the plug-in 
	hash_delete_free(context->plugins, node);

	// Free the plug-in data structures
	free_registered_plugin(plugin);
}

int CP_API cp_uninstall_plugin(cp_context_t *context, const char *id) {
	hnode_t *node;
	int status = CP_OK;

	assert(id != NULL);

	// Look up and unload the plug-in 
	cpi_check_invocation(context, __func__);
	cpi_lock_context(context);
	node = hash_lookup(context->plugins, id);
	if (node != NULL) {
		uninstall_plugin(context, node);
	} else {
		status = CP_ERR_UNKNOWN;
	}
	cpi_unlock_context(context);

	return status;
}

void CP_API cp_uninstall_all_plugins(cp_context_t *context) {
	hscan_t scan;
	hnode_t *node;
	
	cpi_check_invocation(context, __func__);
	cpi_lock_context(context);
	cp_stop_all_plugins(context);
	while (1) {
		hash_scan_begin(&scan, context->plugins);
		if ((node = hash_scan_next(&scan)) != NULL) {
			uninstall_plugin(context, node);
		} else {
			break;
		}
	}
	cpi_unlock_context(context);
}

cp_plugin_info_t * CP_API cp_get_plugin_info(cp_context_t *context, const char *id, int *error) {
	hnode_t *node;
	cp_plugin_info_t *plugin = NULL;
	int status = CP_OK;

	assert(id != NULL);

	// Look up the plug-in and return information 
	cpi_lock_context(context);
	node = hash_lookup(context->plugins, id);
	if (node != NULL) {
		registered_plugin_t *rp = hnode_get(node);
		cpi_use_resource(rp->plugin);
		plugin = rp->plugin;
	} else {
		status = CP_ERR_UNKNOWN;
	}
	cpi_unlock_context(context);

	if (error != NULL) {
		*error = status;
	}
	return plugin;
}

static void dealloc_plugins_info(cp_plugin_info_t **plugins) {
	int i;
	
	assert(plugins != NULL);
	for (i = 0; plugins[i] != NULL; i++) {
		cp_release_info(plugins[i]);
	}
	free(plugins);
}

cp_plugin_info_t ** CP_API cp_get_plugins_info(cp_context_t *context, int *error, int *num) {
	cp_plugin_info_t **plugins = NULL;
	int i, n;
	int status = CP_OK;
	
	cpi_lock_context(context);
	do {
		hscan_t scan;
		hnode_t *node;
		
		// Allocate space for pointer array 
		n = hash_count(context->plugins);
		if ((plugins = malloc(sizeof(cp_plugin_info_t *) * (n + 1))) == NULL) {
			status = CP_ERR_RESOURCE;
			break;
		}
		for (i = 0; i <= n; i++) {
			plugins[i] = NULL;
		}
		
		// Get plug-in information structures 
		hash_scan_begin(&scan, context->plugins);
		i = 0;
		while ((node = hash_scan_next(&scan)) != NULL) {
			registered_plugin_t *rp = hnode_get(node);
			
			assert(i < n);
			cpi_use_resource(rp->plugin);
			plugins[i] = rp->plugin;
			i++;
		}
		
		// Register the array
		status = cpi_register_resource(plugins, (void (*)(void *)) dealloc_plugins_info);
		
	} while (0);
	cpi_unlock_context(context);
	
	// Release resources on error 
	if (status != CP_OK) {
		if (plugins != NULL) {
			dealloc_plugins_info(plugins);
			plugins = NULL;
		}
	}
	
	assert(status != CP_OK || n == 0 || plugins[n - 1] != NULL);
	if (error != NULL) {
		*error = status;
	}
	if (num != NULL && status == CP_OK) {
		*num = n;
	}
	return plugins;
}

cp_plugin_state_t CP_API cp_get_plugin_state(cp_context_t *context, const char *id) {
	cp_plugin_state_t state = CP_PLUGIN_UNINSTALLED;
	hnode_t *hnode;
	
	// Look up the plug-in state 
	cpi_lock_context(context);
	if ((hnode = hash_lookup(context->plugins, id)) != NULL) {
		registered_plugin_t *rp = hnode_get(hnode);
		state = rp->state;
	}
	cpi_unlock_context(context);
	return state;
}
