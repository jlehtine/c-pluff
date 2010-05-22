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
 * Local plug-in loader
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include "cpluff.h"
#include "defines.h"
#include "util.h"
#include "internal.h"


/* ------------------------------------------------------------------------
 * Variables
 * ----------------------------------------------------------------------*/

/// Existing local plug-in loaders
static list_t *local_ploaders = NULL;


/* ------------------------------------------------------------------------
 * Function definitions
 * ----------------------------------------------------------------------*/

static cp_plugin_info_t **lpl_scan_plugins(void *data, cp_context_t *ctx);

CP_C_API cp_plugin_loader_t *cp_create_local_ploader(cp_status_t *error) {
	cp_plugin_loader_t *loader = NULL;
	cp_status_t status = CP_OK;
	
	// Allocate and initialize a new local plug-in loader
	do {
	
		// Allocate memory for the loader
		if ((loader = malloc(sizeof(cp_plugin_loader_t))) == NULL) {
			status = CP_ERR_RESOURCE;
			break;
		}
		
		// Initialize loader
		memset(loader, 0, sizeof(cp_plugin_loader_t));
		loader->data = list_create(LISTCOUNT_T_MAX);
		loader->scan_plugins = lpl_scan_plugins;
		loader->resolve_files = NULL;
		loader->release_plugins = NULL;
		if (loader->data == NULL) {
			status = CP_ERR_RESOURCE;
			break;
		}
	
		// Create a local loader list, if necessary, and add loader to the list
		cpi_lock_framework();
		if (local_ploaders == NULL) {
			if ((local_ploaders = list_create(LISTCOUNT_T_MAX)) == NULL) {
				status = CP_ERR_RESOURCE;
			}
		}
		if (status == CP_OK) {
			lnode_t *node;
			
			if ((node = lnode_create(loader)) == NULL) {
				status = CP_ERR_RESOURCE;
			} else {
				list_append(local_ploaders, node);
			}
		}
		cpi_unlock_framework();
	
	} while (0);
	
	// Release resources on failure
	if (status != CP_OK) {
		if (loader != NULL) {
			cp_destroy_local_ploader(loader);
		}
		loader = NULL;
	}
	
	// Return the final status 
	if (error != NULL) {
		*error = status;
	}
	
	// Return the loader (or NULL on failure)
	return loader;	
}

CP_C_API void cp_destroy_local_ploader(cp_plugin_loader_t *loader) {
	list_t *dirs;
	
	CHECK_NOT_NULL(loader);
	
	dirs = (list_t *) loader->data;
	if (loader->data != NULL) {
		list_process(dirs, NULL, cpi_process_free_ptr);
		list_destroy(dirs);
		loader->data = NULL;
	}
	free(loader);
}

CP_C_API cp_status_t cp_lpl_register_dir(cp_plugin_loader_t *loader, const char *dir) {
	char *d = NULL;
	lnode_t *node = NULL;
	cp_status_t status = CP_OK;
	list_t *dirs;
	
	CHECK_NOT_NULL(loader);
	CHECK_NOT_NULL(dir);
	
	dirs = (list_t *) loader->data;
	do {
	
		// Check if directory has already been registered 
		if (list_find(dirs, dir, (int (*)(const void *, const void *)) strcmp) != NULL) {
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
		list_append(dirs, node);
		
	} while (0);

	// Release resources on failure 
	if (status != CP_OK) {	
		if (d != NULL) {
			free(d);
		}
		if (node != NULL) {
			lnode_destroy(node);
		}
	}
	
	return status;
}

CP_C_API void cp_lpl_unregister_dir(cp_plugin_loader_t *loader, const char *dir) {
	char *d;
	lnode_t *node;
	list_t *dirs;
	
	CHECK_NOT_NULL(loader);
	CHECK_NOT_NULL(dir);
	
	dirs = (list_t *) loader->data;
	node = list_find(dirs, dir, (int (*)(const void *, const void *)) strcmp);
	if (node != NULL) {
		d = lnode_get(node);
		list_delete(dirs, node);
		lnode_destroy(node);
		free(d);
	}
}

CP_C_API void cp_lpl_unregister_dirs(cp_plugin_loader_t *loader) {
	list_t *dirs;
	
	CHECK_NOT_NULL(loader);
	dirs = (list_t *) loader->data;
	list_process(dirs, NULL, cpi_process_free_ptr);
}

static cp_plugin_info_t **lpl_scan_plugins(void *data, cp_context_t *ctx) {
	hash_t *avail_plugins = NULL;
	char *pdir_path = NULL;
	int pdir_path_size = 0;
	list_t *dirs;
	cp_plugin_info_t **plugins = NULL;
	
	CHECK_NOT_NULL(data);
	CHECK_NOT_NULL(ctx);
	
	dirs = (list_t*) data;
	do {
		lnode_t *lnode;
		hscan_t hscan;
		hnode_t *hnode;
		int num_avail_plugins;
		int i;
	
		// Create a hash for available plug-ins 
		if ((avail_plugins = hash_create(HASHCOUNT_T_MAX, (int (*)(const void *, const void *)) strcmp, NULL)) == NULL) {
			break;
		}
	
		// Scan plug-in loaders for available plug-ins 
		lnode = list_first(dirs);
		while (lnode != NULL) {			
			const char *dir_path;
			DIR *dir;
			
			dir_path = lnode_get(lnode);
			dir = opendir(dir_path);
			if (dir != NULL) {
				int dir_path_len;
				struct dirent *de;
				
				dir_path_len = strlen(dir_path);
				if (dir_path[dir_path_len - 1] == CP_FNAMESEP_CHAR) {
					dir_path_len--;
				}
				errno = 0;
				while ((de = readdir(dir)) != NULL) {
					if (de->d_name[0] != '\0' && de->d_name[0] != '.') {
						int pdir_path_len = dir_path_len + 1 + strlen(de->d_name) + 1;
						cp_plugin_info_t *plugin;
						cp_status_t s;
						hnode_t *hnode;

						// Allocate memory for plug-in descriptor path 
						if (pdir_path_size <= pdir_path_len) {
							char *new_pdir_path;
						
							if (pdir_path_size == 0) {
								pdir_path_size = 128;
							}
							while (pdir_path_size <= pdir_path_len) {
								pdir_path_size *= 2;
							}
							new_pdir_path = realloc(pdir_path, pdir_path_size * sizeof(char));
							if (new_pdir_path == NULL) {
								cpi_errorf(ctx, N_("Could not check possible plug-in location %s%c%s due to insufficient system resources."), dir_path, CP_FNAMESEP_CHAR, de->d_name);

								// continue loading plug-ins from other directories 
								continue;
							}
							pdir_path = new_pdir_path;
						}
					
						// Construct plug-in descriptor path 
						strcpy(pdir_path, dir_path);
						pdir_path[dir_path_len] = CP_FNAMESEP_CHAR;
						strcpy(pdir_path + dir_path_len + 1, de->d_name);
							
						// Try to load a plug-in 
						plugin = cp_load_plugin_descriptor(ctx, pdir_path, &s);
						if (plugin == NULL) {
						
							// continue loading plug-ins from other directories 
							continue;
						}
					
						// Insert plug-in to the list of available plug-ins 
						if ((hnode = hash_lookup(avail_plugins, plugin->identifier)) != NULL) {
							cp_plugin_info_t *plugin2 = hnode_get(hnode);
							if (cpi_vercmp(plugin->version, plugin2->version) > 0) {
								hash_delete_free(avail_plugins, hnode);
								cp_release_info(ctx, plugin2);
								hnode = NULL;
							}
						}
						if (hnode == NULL) {
							if (!hash_alloc_insert(avail_plugins, plugin->identifier, plugin)) {
								cpi_errorf(ctx, N_("Plug-in %s version %s could not be loaded due to insufficient system resources."), plugin->identifier, plugin->version);
								cp_release_info(ctx, plugin);

								// continue loading plug-ins from other directories 
								continue;
							}
						}
						
					}
					errno = 0;
				}
				if (errno) {
					cpi_errorf(ctx, N_("Could not read plug-in directory %s: %s"), dir_path, strerror(errno));
					// continue loading plug-ins from other directories 
				}
				closedir(dir);
			} else {
				cpi_errorf(ctx, N_("Could not open plug-in directory %s: %s"), dir_path, strerror(errno));
				// continue loading plug-ins from other directories 
			}
			
			lnode = list_next(dirs, lnode);
		}

		// Construct an array of plug-ins
		num_avail_plugins = hash_count(avail_plugins);
		if ((plugins = malloc(sizeof(cp_plugin_info_t *) * (num_avail_plugins + 1))) == NULL) {
			break;
		}
		hash_scan_begin(&hscan, avail_plugins);
		i = 0;
		while ((hnode = hash_scan_next(&hscan)) != NULL) {
			cp_plugin_info_t *p = hnode_get(hnode);
			hash_scan_delfree(avail_plugins, hnode);
			plugins[i++] = p;
		}
		plugins[i++] = NULL;
		hash_destroy(avail_plugins);
		avail_plugins = NULL;

	} while (0);
	
	// Release resources 
	if (pdir_path != NULL) {
		free(pdir_path);
	}
	if (avail_plugins != NULL) {
		hscan_t hscan;
		hnode_t *hnode;
		
		hash_scan_begin(&hscan, avail_plugins);
		while ((hnode = hash_scan_next(&hscan)) != NULL) {
			cp_plugin_info_t *p = hnode_get(hnode);
			hash_scan_delfree(avail_plugins, hnode);
			cp_release_info(ctx, p);
		}
		hash_destroy(avail_plugins);
	}
	
	return plugins;
}
