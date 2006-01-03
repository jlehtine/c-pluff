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
#include "cpluff.h"
#include "core.h"
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
 * Static function declarations
 * ----------------------------------------------------------------------*/

/**
 * Resolves a plug-in. Resolves all dependencies of a plug-in and loads
 * the plug-in runtime library.
 * 
 * @param plugin the plug-in to be resolved
 * @return CP_OK (0) on success, an error code on failure
 */
static int resolve_plugin(registered_plugin_t *plugin);

/**
 * Recursively resolves a plug-in and its dependencies. Plug-ins with
 * circular dependencies are only preliminary resolved (listeners are
 * not informed).
 * 
 * @param plugin the plug-in to be resolved
 * @param preliminary list of preliminarily resolved plug-ins
 * @return CP_OK (0) or CP_OK_PRELIMINARY (1) on success, an error code on failure
 */
static int resolve_plugin_rec(registered_plugin_t *plugin, list_t *preliminary);

/**
 * Unresolves a preliminarily resolved plug-in.
 * 
 * @param plug-in the plug-in to be unresolved
 */
static void unresolve_preliminary_plugin(registered_plugin_t *plugin);

/**
 * Starts a plug-in.
 * 
 * @param plugin the plug-in to be started
 * @return CP_OK (0) on success, an error code on failure
 */
static int start_plugin(registered_plugin_t *plugin);

/**
 * Stops a plug-in.
 * 
 * @param plugin the plug-in to be stopped
 */
static void stop_plugin(registered_plugin_t *plugin);

/**
 * Unresolves a plug-in.
 * 
 * @param plug-in the plug-in to be unresolved
 */
static void unresolve_plugin(registered_plugin_t *plugin);

/**
 * Unloads a plug-in associated with the specified hash node.
 * 
 * @param node the hash node of the plug-in to be uninstalled
 */
static void unload_plugin(hnode_t *node);

/**
 * Frees any space allocated for a registered plug-in.
 * 
 * @param plugin the plug-in to be freed
 */
static void free_registered_plugin(registered_plugin_t *plugin);


/* ------------------------------------------------------------------------
 * Variables
 * ----------------------------------------------------------------------*/

/** Maps plug-in identifiers to plug-in state structures */
static hash_t *plugins = NULL;

/** List of started plug-ins in the order they were started */
static list_t *started_plugins = NULL;


/* ------------------------------------------------------------------------
 * Function definitions
 * ----------------------------------------------------------------------*/

/* Initiliazation and destruction */

int cpi_init_plugins(void) {
	int status = CP_OK;
	
	do {
	
		/* Initialize data structures */
		plugins = hash_create(HASHCOUNT_T_MAX,
			(int (*)(const void *, const void *)) strcmp, NULL);
		if (plugins == NULL) {
			status = CP_ERR_RESOURCE;
			break;
		}
		started_plugins = list_create(LISTCOUNT_T_MAX);
		if (started_plugins == NULL) {
			hash_destroy(plugins);
			plugins = NULL;
			status = CP_ERR_RESOURCE;
			break;
		}
		
	} while (0);
	if (status != CP_OK) {
		cpi_error(_("Initialization of internal data structures failed due to insufficient resources."));
	}
	return status;
}

void cpi_destroy_plugins(void) {
	
	/* Unload all plug-ins */
	if (plugins != NULL && !hash_isempty(plugins)) {
		cp_unload_all_plugins();
	}
	
	/* Release data structures */
	if (plugins != NULL) {
		assert(hash_isempty(plugins));
		hash_destroy(plugins);
	}
	if (started_plugins != NULL) {
		assert(list_isempty(started_plugins));
		list_destroy(started_plugins);
	}
}


/* Plug-in control */

int cpi_install_plugin(cp_plugin_t *plugin) {
	registered_plugin_t *rp;
	int status = CP_OK;
	cp_plugin_event_t event;

	assert(plugin != NULL);
	
	cpi_acquire_data();
	do {

		/* Check that there is no conflicting plug-in already loaded */
		if (hash_lookup(plugins, plugin->identifier) != NULL) {
			status = CP_ERR_CONFLICT;
			break;
		}

		/* Allocate space for the plug-in state */
		if ((rp = malloc(sizeof(registered_plugin_t))) == NULL) {
			status = CP_ERR_RESOURCE;
			break;
		}
	
		/* Initialize plug-in state */
		rp->plugin = plugin;
		rp->state = CP_PLUGIN_INSTALLED;
		rp->imported = NULL;
		rp->importing = list_create(LISTCOUNT_T_MAX);
		if (rp->importing == NULL) {
			free(rp);
			status = CP_ERR_RESOURCE;
			break;
		}
		rp->runtime_lib = NULL;
		rp->start_func = NULL;
		rp->stop_func = NULL;
		rp->active_operation = 0;
		if (!hash_alloc_insert(plugins, plugin->identifier, rp)) {
			free(rp);
			status = CP_ERR_RESOURCE;
			break;
		}
		
		/* Plug-in installed */
		event.plugin_id = plugin->identifier;
		event.old_state = CP_PLUGIN_UNINSTALLED;
		event.new_state = CP_PLUGIN_INSTALLED;
		cpi_deliver_event(&event);

	} while (0);
	cpi_release_data();

	switch (status) {
		case CP_OK:
			break;
		case CP_ERR_CONFLICT:
			cpi_errorf(_("Plug-in %s could not be installed because a plug-in with the same identifier is already installed."), 
				plugin->identifier);
			break;
		case CP_ERR_RESOURCE:
			cpi_errorf(_("Plug-in %s could not be installed due to insufficient resources."), plugin->identifier);
			break;
		default:
			cpi_errorf(_("Could not register plug-in %s."), plugin->identifier);
			break;
	}
	return status;
}

static int resolve_plugin(registered_plugin_t *plugin) {
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
		cpi_errorf(_("Could not resolve plug-in %s due to insufficient resources."), plugin->plugin->identifier);
		return CP_ERR_RESOURCE;
	}
	
	/* Resolve this plug-in recursively */
	status = resolve_plugin_rec(plugin, preliminary);
	
	/* All plug-ins left in the preliminary list are successfully resolved */
	while((node = list_first(preliminary)) != NULL) {
		registered_plugin_t *rp;
		cp_plugin_event_t event;
		
		rp = lnode_get(node);
		event.plugin_id = rp->plugin->identifier;
		event.old_state = CP_PLUGIN_INSTALLED;
		event.new_state = rp->state;
		cpi_deliver_event(&event);
		list_delete(preliminary, node);
		lnode_destroy(node);
	}
	if (status == CP_OK_PRELIMINARY) {
		status = CP_OK;
	}
	
	return status;
}

static int resolve_plugin_rec
(registered_plugin_t *plugin, list_t *preliminary) {
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
		node = hash_lookup(plugins, plugin->plugin->imports[i].plugin_id);
		if (node != NULL) {
			ip = hnode_get(node);
		} else {
			ip = NULL;
		}
		
		/* TODO: Check plug-in version */
		
		/* Try to resolve the plugin */
		if (ip != NULL) {
			rc = resolve_plugin_rec(ip, preliminary);
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
				cpi_errorf(_("Plug-in %s could not be resolved because it depends on plug-in %s which is not installed."),
					plugin->plugin->identifier,
					plugin->plugin->imports[i].plugin_id);
			} else {
				cpi_errorf(_("Plug-in %s could not be resolved because it depends on plug-in %s which could not be resolved."),
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
			cpi_errorf(_("Could not resolve plug-in %s due to insufficient resources."), plugin->plugin->identifier);
		}
	
		/* 
		 * Unresolve plug-ins which import this plug-in (if there were
		 * circular dependencies while resolving this plug-in)
		 */
		while ((node = list_first(plugin->importing)) != NULL) {
			registered_plugin_t *ip = lnode_get(node);
			unresolve_preliminary_plugin(ip);
			if (cpi_ptrset_remove(preliminary, ip)) {
				cpi_errorf(_("Preliminarily resolved plug-in %s failed to fully resolve because of failed circular dependencies."),
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
		cpi_deliver_event(&event);
	}
	
	return status;
}

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

CP_API(int) cp_start_plugin(const char *id) {
	hnode_t *node;
	registered_plugin_t *plugin;
	int status = CP_OK;

	assert(id != NULL);

	/* Look up and start the plug-in */
	cpi_acquire_data();
	node = hash_lookup(plugins, id);
	if (node != NULL) {
		plugin = hnode_get(node);
		status = start_plugin(plugin);
	} else {
		status = CP_ERR_UNKNOWN;
	}
	cpi_release_data();

	return status;
}

static int start_plugin(registered_plugin_t *plugin) {
	int status;
	cp_plugin_event_t event;
	lnode_t *node;
	
	/* Check if already active */
	if (plugin->state >= CP_PLUGIN_ACTIVE) {
		return CP_OK;
	}
	
	/* Make sure the plug-in is resolved */
	status = resolve_plugin(plugin);
	if (status != CP_OK) {
		return status;
	}
	assert(plugin->state == CP_PLUGIN_RESOLVED);
		
	/* About to start the plug-in */
	event.plugin_id = plugin->plugin->identifier;
	event.old_state = plugin->state;
	event.new_state = plugin->state = CP_PLUGIN_STARTING;
	cpi_deliver_event(&event);
		
	/* Allocate space for the list node */
	node = lnode_create(plugin);
	if (node == NULL) {
		cpi_errorf(_("Could not start plug-in %s due to insufficient resources."),
			plugin->plugin->identifier);
		return CP_ERR_RESOURCE;
	}
		
	/* Start the plug-in */
	if (plugin->start_func != NULL) {
		if (!plugin->start_func()) {
			
			/* Report error */
			cpi_errorf(_("Plug-in %s failed to start due to runtime error."),
				plugin->plugin->identifier);
				
			/* Roll back plug-in state */
			event.old_state = plugin->state;
			event.new_state = plugin->state = CP_PLUGIN_STOPPING;
			cpi_deliver_event(&event);
			if (plugin->stop_func != NULL) {
				plugin->stop_func();
			}
			event.old_state = plugin->state;
			event.new_state = plugin->state = CP_PLUGIN_RESOLVED;
			cpi_deliver_event(&event);
			
			/* Release the list node */
			lnode_destroy(node);

			return CP_ERR_RUNTIME;
		}
	}
		
	/* Plug-in started */
	list_append(started_plugins, node);
	event.old_state = plugin->state;
	event.new_state = plugin->state = CP_PLUGIN_ACTIVE;
	cpi_deliver_event(&event);
	
	return CP_OK;
}

CP_API(int) cp_stop_plugin(const char *id) {
	hnode_t *node;
	registered_plugin_t *plugin;
	int status = CP_OK;

	assert(id != NULL);

	/* Look up and stop the plug-in */
	cpi_acquire_data();
	node = hash_lookup(plugins, id);
	if (node != NULL) {
		plugin = hnode_get(node);
		stop_plugin(plugin);
	} else {
		status = CP_ERR_UNKNOWN;
	}
	cpi_release_data();

	return status;
}

CP_API(void) cp_stop_all_plugins(void) {
	lnode_t *node;
	
	/* Stop the active plug-ins in the reverse order they were started */
	cpi_acquire_data();
	while ((node = list_last(started_plugins)) != NULL) {
		registered_plugin_t *plugin;
		
		plugin = lnode_get(node);
		stop_plugin(plugin);
	}
	cpi_release_data();
}

static void stop_plugin(registered_plugin_t *plugin) {
	cp_plugin_event_t event;
	
	/* Check if already stopped */
	if (plugin->state < CP_PLUGIN_ACTIVE) {
		return;
	}
	assert(plugin->state == CP_PLUGIN_ACTIVE);
		
	/* About to stop the plug-in */
	event.plugin_id = plugin->plugin->identifier;
	event.old_state = plugin->state;
	event.new_state = plugin->state = CP_PLUGIN_STOPPING;
	cpi_deliver_event(&event);
		
	/* Stop the plug-in */
	if (plugin->stop_func != NULL) {
		plugin->stop_func();
	}
		
	/* Plug-in stopped */
	cpi_ptrset_remove(started_plugins, plugin);
	event.old_state = plugin->state;
	event.new_state = plugin->state = CP_PLUGIN_RESOLVED;
	cpi_deliver_event(&event);
}

static void unresolve_plugin(registered_plugin_t *plugin) {
	cp_plugin_event_t event;
	lnode_t *node;

	/* Check if already unresolved */
	if (plugin->state < CP_PLUGIN_RESOLVED) {
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
	
	/* Inform the listeners */
	event.plugin_id = plugin->plugin->identifier;
	event.old_state = CP_PLUGIN_RESOLVED;
	event.new_state = plugin->state;
	cpi_deliver_event(&event);
}

CP_API(int) cp_unload_plugin(const char *id) {
	hnode_t *node;
	int status = CP_OK;

	assert(id != NULL);

	/* Look up and unload the plug-in */
	cpi_acquire_data();
	node = hash_lookup(plugins, id);
	if (node != NULL) {
		unload_plugin(node);
	} else {
		status = CP_ERR_UNKNOWN;
	}
	cpi_release_data();

	return status;
}

CP_API(void) cp_unload_all_plugins(void) {
	hscan_t scan;
	hnode_t *node;
	
	cpi_acquire_data();
	cp_stop_all_plugins();
	do {
		hash_scan_begin(&scan, plugins);
		if ((node = hash_scan_next(&scan)) != NULL) {
			unload_plugin(node);
		}
	} while (node != NULL);
	cpi_release_data();
}

static void unload_plugin(hnode_t *node) {
	registered_plugin_t *plugin;
	cp_plugin_event_t event;
	
	/* Make sure the plug-in is not in resolved state */
	plugin = (registered_plugin_t *) hnode_get(node);
	unresolve_plugin(plugin);

	/* Unregister the plug-in */
	hash_delete_free(plugins, node);

	/* Plug-in unregistered */
	event.plugin_id = plugin->plugin->identifier;
	event.old_state = plugin->state;
	event.new_state = CP_PLUGIN_UNINSTALLED;
	cpi_deliver_event(&event);

	/* Free the plug-in data structures */
	free_registered_plugin(plugin);
	
}

static void free_registered_plugin(registered_plugin_t *plugin) {
	cpi_free_plugin(plugin->plugin);
	
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

void cpi_free_plugin(cp_plugin_t *plugin) {
	assert(plugin != NULL);
	if (plugin->path != NULL) {
		free(plugin->path);
	}
	if (plugin->imports != NULL) {
		free(plugin->imports);
	}
	if (plugin->ext_points != NULL) {
		free(plugin->ext_points);
	}
	if (plugin->extensions != NULL) {
		free(plugin->extensions);
	}
	free(plugin);
}
