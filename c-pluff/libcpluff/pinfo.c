/*-------------------------------------------------------------------------
 * C-Pluff, a plug-in framework for C
 * Copyright 2006 Johannes Lehtinen
 *-----------------------------------------------------------------------*/

/** @file
 * Plug-in information functions
 */

#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "../kazlib/hash.h"
#include "cpluff.h"
#include "defines.h"
#include "util.h"
#include "internal.h"


/* ------------------------------------------------------------------------
 * Data types
 * ----------------------------------------------------------------------*/

typedef struct info_resource_t info_resource_t;

/// Contains information about a dynamically allocated information object
struct info_resource_t {

	/// Pointer to the resource
	void *resource;	
	
	/// Usage count for the resource
	int usage_count;
	
	/// Deallocation function
	cpi_dealloc_func_t dealloc_func;
};


/* ------------------------------------------------------------------------
 * Variables
 * ----------------------------------------------------------------------*/

/// Map of in-use dynamic resources
static hash_t *infos = NULL;


/* ------------------------------------------------------------------------
 * Function definitions
 * ----------------------------------------------------------------------*/

// General information object management

CP_HIDDEN cp_status_t cpi_register_info(void *res, cpi_dealloc_func_t df) {
	cp_status_t status = CP_OK;
	info_resource_t *ir = NULL;
	
	do {
		if ((ir = malloc(sizeof(info_resource_t))) == NULL) {
			status = CP_ERR_RESOURCE;
			break;
		}
		ir->resource = res;
		ir->usage_count = 1;
		ir->dealloc_func = df;
		cpi_lock_framework();
		if (infos == NULL
			&& (infos = hash_create(HASHCOUNT_T_MAX, cpi_comp_ptr, cpi_hashfunc_ptr)) == NULL) {
			status = CP_ERR_RESOURCE;
			break;
		}
		if (!hash_alloc_insert(infos, res, ir)) {
			status = CP_ERR_RESOURCE;
		}
		cpi_unlock_framework();
		
	} while (0);
	
	// Release resources on failure
	if (status != CP_OK) {
		if (ir != NULL) {
			free(ir);
		}
	}
	
	return status;
}

CP_HIDDEN void cpi_use_info(void *res) {
	hnode_t *node;
	
	cpi_lock_framework();
	if (infos != NULL
		&& (node = hash_lookup(infos, res)) != NULL) {
		info_resource_t *ir = hnode_get(node);
		ir->usage_count++;
	} else {
		cpi_fatalf(_("Could not increase usage count on unknown information object."));
	}
	cpi_unlock_framework();
}

CP_C_API void cp_release_info(void *info) {
	hnode_t *node;
	
	CHECK_NOT_NULL(info);
	cpi_lock_framework();
	if (infos != NULL
		&& (node = hash_lookup(infos, info)) != NULL) {
		info_resource_t *ir = hnode_get(node);
		assert(ir != NULL && info == ir->resource);
		if (--ir->usage_count == 0) {
			hash_delete_free(infos, node);
			ir->dealloc_func(info);
			free(ir);
		}
	} else {
		cpi_fatalf(_("Trying to release unregistered information object %p."), info);
	}
	cpi_unlock_framework();
}

CP_HIDDEN void cpi_destroy_all_infos(void) {
	cpi_lock_framework();
	if (infos != NULL) {
		hscan_t scan;
		hnode_t *node;
		
		hash_scan_begin(&scan, infos);
		while ((node = hash_scan_next(&scan)) != NULL) {
			info_resource_t *ir = hnode_get(node);
			
			hash_scan_delfree(infos, node);
			ir->dealloc_func(ir->resource);
			free(ir);
		}
		hash_destroy(infos);
		infos = NULL;
	}
	cpi_unlock_framework();
}


// Information acquiring functions

CP_C_API cp_plugin_info_t * cp_get_plugin_info(cp_context_t *context, const char *id, cp_status_t *error) {
	hnode_t *node;
	cp_plugin_info_t *plugin = NULL;
	cp_status_t status = CP_OK;

	CHECK_NOT_NULL(context);
	CHECK_NOT_NULL(id);

	// Look up the plug-in and return information 
	cpi_lock_context(context);
	cpi_check_invocation(context, CPI_CF_LOGGER, __func__);
	node = hash_lookup(context->env->plugins, id);
	if (node != NULL) {
		cp_plugin_t *rp = hnode_get(node);
		cpi_use_info(rp->plugin);
		plugin = rp->plugin;
	} else {
		cpi_warnf(context, _("Could not return information about unknown plug-in %s."), id);
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

CP_C_API cp_plugin_info_t ** cp_get_plugins_info(cp_context_t *context, cp_status_t *error, int *num) {
	cp_plugin_info_t **plugins = NULL;
	int i, n;
	cp_status_t status = CP_OK;
	
	CHECK_NOT_NULL(context);
	
	cpi_lock_context(context);
	cpi_check_invocation(context, CPI_CF_LOGGER, __func__);
	do {
		hscan_t scan;
		hnode_t *node;
		
		// Allocate space for pointer array 
		n = hash_count(context->env->plugins);
		if ((plugins = malloc(sizeof(cp_plugin_info_t *) * (n + 1))) == NULL) {
			status = CP_ERR_RESOURCE;
			break;
		}
		
		// Get plug-in information structures 
		hash_scan_begin(&scan, context->env->plugins);
		i = 0;
		while ((node = hash_scan_next(&scan)) != NULL) {
			cp_plugin_t *rp = hnode_get(node);
			
			assert(i < n);
			cpi_use_info(rp->plugin);
			plugins[i] = rp->plugin;
			i++;
		}
		plugins[i] = NULL;
		
		// Register the array
		status = cpi_register_info(plugins, (void (*)(void *)) dealloc_plugins_info);
		
	} while (0);
	cpi_unlock_context(context);
	
	// Release resources on error 
	if (status != CP_OK) {
		cpi_error(context, _("Plug-in information could not be returned due to insufficient memory."));
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

CP_C_API cp_plugin_state_t cp_get_plugin_state(cp_context_t *context, const char *id) {
	cp_plugin_state_t state = CP_PLUGIN_UNINSTALLED;
	hnode_t *hnode;
	
	CHECK_NOT_NULL(context);
	CHECK_NOT_NULL(id);
	
	// Look up the plug-in state 
	cpi_lock_context(context);
	cpi_check_invocation(context, CPI_CF_LOGGER, __func__);
	if ((hnode = hash_lookup(context->env->plugins, id)) != NULL) {
		cp_plugin_t *rp = hnode_get(hnode);
		state = rp->state;
	}
	cpi_unlock_context(context);
	return state;
}

static void dealloc_ext_points_info(cp_ext_point_t **ext_points) {
	int i;
	
	assert(ext_points != NULL);
	for (i = 0; ext_points[i] != NULL; i++) {
		cp_release_info(ext_points[i]->plugin);
	}
	free(ext_points);
}

CP_C_API cp_ext_point_t ** cp_get_ext_points_info(cp_context_t *context, cp_status_t *error, int *num) {
	cp_ext_point_t **ext_points = NULL;
	int i, n;
	cp_status_t status = CP_OK;
	
	CHECK_NOT_NULL(context);
	
	cpi_lock_context(context);
	cpi_check_invocation(context, CPI_CF_LOGGER, __func__);
	do {
		hscan_t scan;
		hnode_t *node;
		
		// Allocate space for pointer array 
		n = hash_count(context->env->ext_points);
		if ((ext_points = malloc(sizeof(cp_ext_point_t *) * (n + 1))) == NULL) {
			status = CP_ERR_RESOURCE;
			break;
		}
		
		// Get extension point information structures 
		hash_scan_begin(&scan, context->env->ext_points);
		i = 0;
		while ((node = hash_scan_next(&scan)) != NULL) {
			cp_ext_point_t *ep = hnode_get(node);
			
			assert(i < n);
			cpi_use_info(ep->plugin);
			ext_points[i] = ep;
			i++;
		}
		ext_points[i] = NULL;
		
		// Register the array
		status = cpi_register_info(ext_points, (void (*)(void *)) dealloc_ext_points_info);
		
	} while (0);
	cpi_unlock_context(context);
	
	// Release resources on error 
	if (status != CP_OK) {
		cpi_error(context, _("Extension point information could not be returned due to insufficient memory."));
		if (ext_points != NULL) {
			dealloc_ext_points_info(ext_points);
			ext_points = NULL;
		}
	}
	
	assert(status != CP_OK || n == 0 || ext_points[n - 1] != NULL);
	if (error != NULL) {
		*error = status;
	}
	if (num != NULL && status == CP_OK) {
		*num = n;
	}
	return ext_points;
}

static void dealloc_extensions_info(cp_extension_t **extensions) {
	int i;
	
	assert(extensions != NULL);
	for (i = 0; extensions[i] != NULL; i++) {
		cp_release_info(extensions[i]->plugin);
	}
	free(extensions);
}

CP_C_API cp_extension_t ** cp_get_extensions_info(cp_context_t *context, const char *extpt_id, cp_status_t *error, int *num) {
	cp_extension_t **extensions = NULL;
	int i, n;
	cp_status_t status = CP_OK;
	
	CHECK_NOT_NULL(context);
	
	cpi_lock_context(context);
	cpi_check_invocation(context, CPI_CF_LOGGER, __func__);
	do {
		hscan_t scan;
		hnode_t *hnode;

		// Count the number of extensions
		if (extpt_id != NULL) {
			if ((hnode = hash_lookup(context->env->extensions, extpt_id)) != NULL) {
				n = list_count((list_t *) hnode_get(hnode));
			} else {
				n = 0;
			}
		} else {
			hscan_t scan;
			
			n = 0;
			hash_scan_begin(&scan, context->env->extensions);
			while ((hnode = hash_scan_next(&scan)) != NULL) {
				n += list_count((list_t *) hnode_get(hnode));
			}
		}
		
		// Allocate space for pointer array 
		if ((extensions = malloc(sizeof(cp_extension_t *) * (n + 1))) == NULL) {
			status = CP_ERR_RESOURCE;
			break;
		}
		
		// Get extension information structures 
		hash_scan_begin(&scan, context->env->extensions);
		i = 0;
		while ((hnode = hash_scan_next(&scan)) != NULL) {
			list_t *el = hnode_get(hnode);
			lnode_t *lnode;
			
			lnode = list_first(el);
			while (lnode != NULL) {
				cp_extension_t *e = lnode_get(lnode);
				
				assert(i < n);
				cpi_use_info(e->plugin);
				extensions[i] = e;
				i++;
				lnode = list_next(el, lnode);
			}
		}
		extensions[i] = NULL;
		
		// Register the array
		status = cpi_register_info(extensions, (void (*)(void *)) dealloc_extensions_info);
		
	} while (0);
	cpi_unlock_context(context);
	
	// Release resources on error 
	if (status != CP_OK) {
		cpi_error(context, _("Extension information could not be returned due to insufficient memory."));
		if (extensions != NULL) {
			dealloc_extensions_info(extensions);
			extensions = NULL;
		}
	}
	
	assert(status != CP_OK || n == 0 || extensions[n - 1] != NULL);
	if (error != NULL) {
		*error = status;
	}
	if (num != NULL && status == CP_OK) {
		*num = n;
	}
	return extensions;
}

static cp_cfg_element_t * lookup_cfg_element(cp_cfg_element_t *base, const char *path, int len) {
	int start = 0;
	
	CHECK_NOT_NULL(base);
	CHECK_NOT_NULL(path);
	
	// Traverse the path
	while (base != NULL && path[start] != '\0' && (len == -1 || start < len)) {
		int end = start;
		while (path[end] != '\0' && path[end] != '/' && (len == -1 || end < len))
			end++;
		if (end - start == 2 && !strncmp(path + start, "..", 2)) {
			base = base->parent;
		} else {
			int i;
			int found = 0;
			
			for (i = 0; !found && i < base->num_children; i++) {
				cp_cfg_element_t *e = base->children + i;
				if (end - start == strlen(e->name)
					&& !strncmp(path + start, e->name, end - start)) {
					base = e;
					found = 1;
				}
			}
			if (!found) {
				base = NULL;
			}
		}
		start = end;
		if (path[start] == '/') {
			start++;
		}
	}
	return base;
}

CP_C_API cp_cfg_element_t * cp_lookup_cfg_element(cp_cfg_element_t *base, const char *path) {
	return lookup_cfg_element(base, path, -1);
}

CP_C_API char * cp_lookup_cfg_value(cp_cfg_element_t *base, const char *path) {
	cp_cfg_element_t *e;
	const char *attr;
	
	CHECK_NOT_NULL(base);
	CHECK_NOT_NULL(path);
	
	if ((attr = strrchr(path, '@')) == NULL) {
		e = lookup_cfg_element(base, path, -1);
	} else {
		e = lookup_cfg_element(base, path, attr - path);
		attr++;
	}
	if (e != NULL) {
		if (attr == NULL) {
			return e->value;
		} else {
			int i;
			
			for (i = 0; i < e->num_atts; i++) {
				if (!strcmp(attr, e->atts[2*i])) {
					return e->atts[2*i + 1];
				}
			}
			return NULL;
		}
	} else {
		return NULL;
	}
}
