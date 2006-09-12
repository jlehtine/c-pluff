/*-------------------------------------------------------------------------
 * C-Pluff, a plug-in framework for C
 * Copyright 2005-2006 Johannes Lehtinen
 *-----------------------------------------------------------------------*/

/*
 * Plug-in control functionality
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stddef.h>
#ifdef HAVE_DLOPEN
#include <dlfcn.h>
#endif
#include "cpluff.h"
#include "context.h"
#include "pcontrol.h"
#include "util.h"
#include "kazlib/list.h"
#include "kazlib/hash.h"


/* ------------------------------------------------------------------------
 * Data types
 * ----------------------------------------------------------------------*/

/* Forward type definitions */
typedef struct registered_plugin_t registered_plugin_t;

/** Stores the state of a registered plug-in. */
struct registered_plugin_t {
	
	/** Plug-in information */
	cp_plugin_t *plugin;
	
	/** Use count for plug-in information */
	unsigned int use_count;
	
	/** The current state of the plug-in */
	cp_plugin_state_t state;
	
	/** The set of imported plug-ins, or NULL if not resolved */
	list_t *imported;
	
	/** The set of plug-ins importing this plug-in */
	list_t *importing;
	
	/** The runtime library handle, or NULL if not resolved */
	void *runtime_lib;
	
	/** The start function, or NULL if none or not resolved */
	cp_start_t start_func;
	
	/** The stop function, or NULL if none or not resolved */
	cp_stop_t stop_func;
	
	/**
	 * Whether there is currently an active operation on this plug-in (to
	 * break loops in the dependency graph)
	 */
	int active_operation;
};


/* ------------------------------------------------------------------------
 * Function definitions
 * ----------------------------------------------------------------------*/

/* Plug-in control */

int CP_LOCAL cpi_install_plugin(cp_context_t *context, cp_plugin_t *plugin, unsigned int use_count) {
	registered_plugin_t *rp;
	int status = CP_OK;
	cp_plugin_event_t event;

	assert(plugin != NULL);
	
	cpi_lock_context(context);
	do {

		/* Check that there is no conflicting plug-in already loaded */
		if (hash_lookup(context->plugins, plugin->identifier) != NULL) {
			status = CP_ERR_CONFLICT;
			break;
		}

		/* Allocate space for the plug-in state */
		if ((rp = malloc(sizeof(registered_plugin_t))) == NULL) {
			status = CP_ERR_RESOURCE;
			break;
		}
	
		/* Initialize plug-in state */
		memset(rp, 0, sizeof(registered_plugin_t));
		rp->use_count = use_count;
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
		if (use_count != 0) {
			if (!hash_alloc_insert(context->used_plugins, rp->plugin, rp)) {
				hnode_t *node;
				
				node = hash_lookup(context->plugins, plugin->identifier);
				assert(node != NULL);
				hash_delete_free(context->plugins, node);
				list_destroy(rp->importing);
				free(rp);
				status = CP_ERR_RESOURCE;
				break;
			}
		}
		
		/* Plug-in installed */
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
				_("Plug-in %s could not be installed due to insufficient resources."), plugin->identifier);
			break;
		default:
			cpi_errorf(context,
				_("Could not register plug-in %s."), plugin->identifier);
			break;
	}
	return status;
}

/**
 * Unresolves a preliminarily resolved plug-in.
 * 
 * @param plug-in the plug-in to be unresolved
 */
static void unresolve_preliminary_plugin(registered_plugin_t *plugin) {
	lnode_t *node;
	
	/* Break circular dependencies */
	if (plugin->active_operation) {
		return;
	}

	/* Unresolve plug-ins which import this plug-in */
	plugin->active_operation = 1;
	while ((node = list_first(plugin->importing)) != NULL) {
		registered_plugin_t *ip = lnode_get(node);
		unresolve_preliminary_plugin(ip);
	}
	plugin->active_operation = 0;
		
	/* Remove references to imported plug-ins */
	while ((node = list_first(plugin->imported)) != NULL) {
		registered_plugin_t *ip = lnode_get(node);
		cpi_ptrset_remove(ip->importing, plugin);
		list_delete(plugin->imported, node);
		lnode_destroy(node);
	}
	list_destroy(plugin->imported);
	plugin->imported = NULL;
		
	/* Update plug-in state */
	plugin->state = CP_PLUGIN_INSTALLED;	
}

/**
 * Recursively resolves a plug-in and its dependencies. Plug-ins with
 * circular dependencies are only preliminary resolved (listeners are
 * not informed).
 * 
 * @param context the plug-in context
 * @param plugin the plug-in to be resolved
 * @param preliminary list of preliminarily resolved plug-ins
 * @return CP_OK (0) or CP_OK_PRELIMINARY (1) on success, an error code on failure
 */
static int resolve_plugin_rec
(cp_context_t *context, registered_plugin_t *plugin, list_t *preliminary) {
	int i;
	int status = CP_OK;
	
	/* Check if already resolved */
	if (plugin->state >= CP_PLUGIN_RESOLVED) {
		return CP_OK;
	}
	assert(plugin->state == CP_PLUGIN_INSTALLED);
	
	/* Check if being resolved (due to circular dependency) */
	if (plugin->active_operation) {
		return CP_OK_PRELIMINARY;
	}	

	/* Allocate space for the import information */
	assert(plugin->imported == NULL);
	plugin->imported = list_create(LISTCOUNT_T_MAX);
	if (plugin->imported == NULL) {
		status = CP_ERR_RESOURCE;
	}
	
	/* Check that all imported plug-ins are present and resolved */
	plugin->active_operation = 1;
	for (i = 0;
		(status == CP_OK || status == CP_OK_PRELIMINARY)
			&& i < plugin->plugin->num_imports;
		i++) {
		registered_plugin_t *ip;
		hnode_t *node;
		int rc;
		
		/* Lookup the plug-in */
		node = hash_lookup(context->plugins, plugin->plugin->imports[i].plugin_id);
		if (node != NULL) {
			ip = hnode_get(node);
		} else {
			ip = NULL;
		}
		
		/* TODO: Check plug-in version */
		
		/* Try to resolve the plugin */
		if (ip != NULL) {
			rc = resolve_plugin_rec(context, ip, preliminary);
		} else {
			rc = CP_OK;
		}
		
		/* If import was succesful, register the dependency */
		if (ip != NULL && (rc == CP_OK || rc == CP_OK_PRELIMINARY)) {
			if (!cpi_ptrset_add(plugin->imported, ip)) {
				status = CP_ERR_RESOURCE;
			} else if (!cpi_ptrset_add(ip->importing, plugin)) {
				status = CP_ERR_RESOURCE;
			}
			if (rc == CP_OK_PRELIMINARY) {
				status = CP_OK_PRELIMINARY;
			}
		}
		
		/* Otherwise check if the failed import was mandatory */
		else if (!(plugin->plugin->imports[i].optional)) {
			if (plugin == NULL) {
				cpi_errorf(context,
					_("Plug-in %s could not be resolved because it depends on plug-in %s which is not installed."),
					plugin->plugin->identifier,
					plugin->plugin->imports[i].plugin_id);
			} else {
				cpi_errorf(context,
					_("Plug-in %s could not be resolved because it depends on plug-in %s which could not be resolved."),
					plugin->plugin->identifier,
					plugin->plugin->imports[i].plugin_id);
			}
			status = CP_ERR_DEPENDENCY;
		}
		
	}

	/* Update state but postpone listener update on preliminary success */
	if (status == CP_OK_PRELIMINARY) {
		if (cpi_ptrset_add(preliminary, plugin)) {
			plugin->state = CP_PLUGIN_RESOLVED;
		} else {
			status = CP_ERR_RESOURCE;
		}
	}
	
	/* Clean up on failure */
	if (status != CP_OK && status != CP_OK_PRELIMINARY) {
		lnode_t *node;
		
		/* Report possible resource allocation problem */
		if (status == CP_ERR_RESOURCE) {
			cpi_errorf(context,
				_("Could not resolve plug-in %s due to insufficient resources."), plugin->plugin->identifier);
		}
	
		/* 
		 * Unresolve plug-ins which import this plug-in (if there were
		 * circular dependencies while resolving this plug-in)
		 */
		while ((node = list_first(plugin->importing)) != NULL) {
			registered_plugin_t *ip = lnode_get(node);
			unresolve_preliminary_plugin(ip);
			if (cpi_ptrset_remove(preliminary, ip)) {
				cpi_errorf(context,
					_("Preliminarily resolved plug-in %s failed to fully resolve because of failed circular dependencies."),
					ip->plugin->identifier);
			}
			assert(!cpi_ptrset_contains(plugin->importing, ip));
		}
		
		/* Remove references to imported plug-ins */
		while ((node = list_first(plugin->imported)) != NULL) {
			registered_plugin_t *ip = lnode_get(node);
			cpi_ptrset_remove(ip->importing, plugin);
			list_delete(plugin->imported, node);
			lnode_destroy(node);
		}
		list_destroy(plugin->imported);
		plugin->imported = NULL;
	}	
	plugin->active_operation = 0;
	
	/* Update state and inform listeners on definite success */
	if (status == CP_OK) {
		cp_plugin_event_t event;
		
		event.plugin_id = plugin->plugin->identifier;
		event.old_state = plugin->state;
		event.new_state = plugin->state = CP_PLUGIN_RESOLVED;
		cpi_deliver_event(context, &event);
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
	list_t *preliminary;
	int status;
	lnode_t *node;
	
	assert(plugin != NULL);
	
	/* Check if already resolved */
	if (plugin->state >= CP_PLUGIN_RESOLVED) {
		return CP_OK;
	}
	
	/* Create a list for preliminarily resolved plug-ins */
	preliminary = list_create(LISTCOUNT_T_MAX);
	if (preliminary == NULL) {
		cpi_errorf(context,
			_("Could not resolve plug-in %s due to insufficient resources."), plugin->plugin->identifier);
		return CP_ERR_RESOURCE;
	}
	
	/* Resolve this plug-in recursively */
	status = resolve_plugin_rec(context, plugin, preliminary);
	
	/* All plug-ins left in the preliminary list are successfully resolved */
	while((node = list_first(preliminary)) != NULL) {
		registered_plugin_t *rp;
		cp_plugin_event_t event;
		
		rp = lnode_get(node);
		event.plugin_id = rp->plugin->identifier;
		event.old_state = CP_PLUGIN_INSTALLED;
		event.new_state = rp->state;
		cpi_deliver_event(context, &event);
		list_delete(preliminary, node);
		lnode_destroy(node);
	}
	if (status == CP_OK_PRELIMINARY) {
		status = CP_OK;
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
	
	/* Check if already active */
	if (plugin->state >= CP_PLUGIN_ACTIVE) {
		return CP_OK;
	}
	
	/* Make sure the plug-in is resolved */
	status = resolve_plugin(context, plugin);
	if (status != CP_OK) {
		return status;
	}
	assert(plugin->state == CP_PLUGIN_RESOLVED);
		
	/* About to start the plug-in */
	event.plugin_id = plugin->plugin->identifier;
	event.old_state = plugin->state;
	event.new_state = plugin->state = CP_PLUGIN_STARTING;
	cpi_deliver_event(context, &event);
		
	/* Allocate space for the list node */
	node = lnode_create(plugin);
	if (node == NULL) {
		cpi_errorf(context,
			_("Could not start plug-in %s due to insufficient resources."),
			plugin->plugin->identifier);
		return CP_ERR_RESOURCE;
	}
		
	/* Start the plug-in */
	if (plugin->start_func != NULL) {
		if (!plugin->start_func(context)) {
			
			/* Report error */
			cpi_errorf(context,
				_("Plug-in %s failed to start due to runtime error."),
				plugin->plugin->identifier);
				
			/* Roll back plug-in state */
			event.old_state = plugin->state;
			event.new_state = plugin->state = CP_PLUGIN_STOPPING;
			cpi_deliver_event(context, &event);
			if (plugin->stop_func != NULL) {
				plugin->stop_func(context);
			}
			event.old_state = plugin->state;
			event.new_state = plugin->state = CP_PLUGIN_RESOLVED;
			cpi_deliver_event(context, &event);
			
			/* Release the list node */
			lnode_destroy(node);

			return CP_ERR_RUNTIME;
		}
	}
		
	/* Plug-in started */
	list_append(context->started_plugins, node);
	event.old_state = plugin->state;
	event.new_state = plugin->state = CP_PLUGIN_ACTIVE;
	cpi_deliver_event(context, &event);
	
	return CP_OK;
}

int CP_API cp_start_plugin(cp_context_t *context, const char *id) {
	hnode_t *node;
	registered_plugin_t *plugin;
	int status = CP_OK;

	assert(id != NULL);

	/* Look up and start the plug-in */
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
 * @param plugin the plug-in to be stopped
 */
static void stop_plugin(registered_plugin_t *plugin) {
	cp_plugin_event_t event;
	cp_context_t *context = plugin->plugin->context;
	
	/* Check if already stopped */
	if (plugin->state < CP_PLUGIN_ACTIVE) {
		return;
	}
	assert(plugin->state == CP_PLUGIN_ACTIVE);
		
	/* About to stop the plug-in */
	event.plugin_id = plugin->plugin->identifier;
	event.old_state = plugin->state;
	event.new_state = plugin->state = CP_PLUGIN_STOPPING;
	cpi_deliver_event(context, &event);
		
	/* Stop the plug-in */
	if (plugin->stop_func != NULL) {
		plugin->stop_func(context);
	}
		
	/* Plug-in stopped */
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

	/* Look up and stop the plug-in */
	cpi_lock_context(context);
	node = hash_lookup(context->plugins, id);
	if (node != NULL) {
		plugin = hnode_get(node);
		stop_plugin(plugin);
	} else {
		status = CP_ERR_UNKNOWN;
	}
	cpi_unlock_context(context);

	return status;
}

void CP_API cp_stop_all_plugins(cp_context_t *context) {
	lnode_t *node;
	
	/* Stop the active plug-ins in the reverse order they were started */
	cpi_lock_context(context);
	while ((node = list_last(context->started_plugins)) != NULL) {
		registered_plugin_t *plugin;
		
		plugin = lnode_get(node);
		stop_plugin(plugin);
	}
	cpi_unlock_context(context);
}

/**
 * Unresolves a plug-in.
 * 
 * @param plug-in the plug-in to be unresolved
 */
static void unresolve_plugin(registered_plugin_t *plugin) {
	cp_plugin_event_t event;
	lnode_t *node;

	/* Check if already unresolved */
	if (plugin->state <= CP_PLUGIN_INSTALLED) {
		return;
	}

	/* Break circular dependencies */
	if (plugin->active_operation) {
		return;
	}

	/* Make sure the plug-in is not active */
	stop_plugin(plugin);
	assert(plugin->state == CP_PLUGIN_RESOLVED);
	
	/* Unresolve plug-ins which import this plug-in */
	plugin->active_operation = 1;
	while ((node = list_first(plugin->importing)) != NULL) {
		registered_plugin_t *ip = lnode_get(node);
		unresolve_plugin(ip);
		assert(!cpi_ptrset_contains(plugin->importing, ip));
	}
	plugin->active_operation = 0;
		
	/* Remove references to imported plug-ins */
	while ((node = list_first(plugin->imported)) != NULL) {
		registered_plugin_t *ip = lnode_get(node);
		cpi_ptrset_remove(ip->importing, plugin);
		list_delete(plugin->imported, node);
		lnode_destroy(node);
	}
	list_destroy(plugin->imported);
	plugin->imported = NULL;
	
	/* Reset pointers */
	plugin->start_func = NULL;
	plugin->stop_func = NULL;
	if (plugin->runtime_lib != NULL) {
#ifdef HAVE_DLOPEN
		dlclose(plugin->runtime_lib);
#endif
		plugin->runtime_lib = NULL;
	}
	
	/* Inform the listeners */
	event.plugin_id = plugin->plugin->identifier;
	event.old_state = plugin->state;
	event.new_state = plugin->state = CP_PLUGIN_INSTALLED;
	cpi_deliver_event(plugin->plugin->context, &event);
}

// TODO
static void free_plugin_import_content(cp_plugin_import_t *import) {
	assert(import != NULL);
	free(import->plugin_id);
	free(import->version);
}

// TODO
static void free_ext_point_content(cp_ext_point_t *ext_point) {
	free(ext_point->name);
	free(ext_point->local_id);
	free(ext_point->global_id);
	free(ext_point->schema_path);
}

// TODO
static void free_extension_content(cp_extension_t *extension) {
	free(extension->name);
	free(extension->local_id);
	free(extension->global_id);
	free(extension->ext_point_id);
}

// TODO
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

void CP_LOCAL cpi_free_plugin(cp_plugin_t *plugin) {
	int i;
	
	assert(plugin != NULL);
	free(plugin->name);
	free(plugin->identifier);
	free(plugin->version);
	free(plugin->provider_name);
	free(plugin->path);
	for (i = 0; i < plugin->num_imports; i++) {
		free_plugin_import_content(plugin->imports + i);
	}
	free(plugin->imports);
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
	cpi_free_plugin(plugin->plugin);

	assert(plugin != NULL);
	
	/* Release data structures */
	if (plugin->importing != NULL) {
		
		/* This should be empty because the plug-in should be unresolved */
		assert(list_isempty(plugin->importing));
		
		list_destroy(plugin->importing);
	}

	/* This should be null because the plug-in should be unresolved */
	assert(plugin->imported == NULL);

	free(plugin);
}

/**
 * Unloads a plug-in associated with the specified hash node.
 * 
 * @param node the hash node of the plug-in to be uninstalled
 */
static void unload_plugin(hnode_t *node) {
	registered_plugin_t *plugin;
	cp_plugin_event_t event;
	
	/* Check if already uninstalled */
	plugin = (registered_plugin_t *) hnode_get(node);
	if (plugin->state <= CP_PLUGIN_UNINSTALLED) {
		return;
	}
	
	/* Make sure the plug-in is not in resolved state */
	unresolve_plugin(plugin);

	/* Plug-in uninstalled */
	event.plugin_id = plugin->plugin->identifier;
	event.old_state = plugin->state;
	event.new_state = plugin->state = CP_PLUGIN_UNINSTALLED;
	cpi_deliver_event(plugin->plugin->context, &event);
	
	/* Unregister the plug-in */
	hash_delete_free(plugin->plugin->context->plugins, node);

	/* Free the plug-in data structures if they are not needed anymore */
	if (plugin->use_count == 0) {
		free_registered_plugin(plugin);
	}
	
}

int CP_API cp_unload_plugin(cp_context_t *context, const char *id) {
	hnode_t *node;
	int status = CP_OK;

	assert(id != NULL);

	/* Look up and unload the plug-in */
	cpi_lock_context(context);
	node = hash_lookup(context->plugins, id);
	if (node != NULL) {
		unload_plugin(node);
	} else {
		status = CP_ERR_UNKNOWN;
	}
	cpi_unlock_context(context);

	return status;
}

void CP_API cp_unload_all_plugins(cp_context_t *context) {
	hscan_t scan;
	hnode_t *node;
	
	cpi_lock_context(context);
	cp_stop_all_plugins(context);
	do {
		hash_scan_begin(&scan, context->plugins);
		if ((node = hash_scan_next(&scan)) != NULL) {
			unload_plugin(node);
		}
	} while (node != NULL);
	cpi_unlock_context(context);
}

const cp_plugin_t * CP_API cp_get_plugin(cp_context_t *context, const char *id, int *error) {
	hnode_t *node;
	const cp_plugin_t *plugin;
	int status = CP_OK;

	assert(id != NULL);

	/* Look up the plug-in and return information */
	cpi_lock_context(context);
	node = hash_lookup(context->plugins, id);
	if (node != NULL) {
		registered_plugin_t *rp = hnode_get(node);
		
		if (!hash_alloc_insert(context->used_plugins, rp->plugin, rp)) {
			plugin = NULL;
			status = CP_ERR_RESOURCE;
			cpi_error(context, _("Insufficient resources to return plug-in information."));
		} else {
			rp->use_count++;
			plugin = rp->plugin;
		}
	} else {
		plugin = NULL;
		status = CP_ERR_UNKNOWN;
	}
	cpi_unlock_context(context);

	if (error != NULL) {
		*error = status;
	}
	return plugin;
}

void CP_API cp_release_plugin(const cp_plugin_t *plugin) {
	registered_plugin_t *rp;
	hnode_t *node;
	
	assert(plugin != NULL);
	
	/* Look up the plug-in and return information */
	cpi_lock_context(plugin->context);
	node = hash_lookup(plugin->context->used_plugins, plugin);
	if (node == NULL) {
		cpi_errorf(plugin->context,
			_("Attempt to release plug-in %s which is not in use."),
			plugin->identifier);
		return;
	}
	rp = (registered_plugin_t *) hnode_get(node);
	rp->use_count--;
	if (rp->use_count == 0) {
		hash_delete_free(plugin->context->used_plugins, node);
		if (rp->state == CP_PLUGIN_UNINSTALLED) {
			free_registered_plugin(rp);
		}
	}
	cpi_unlock_context(plugin->context);
}

