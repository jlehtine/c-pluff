/*-------------------------------------------------------------------------
 * C-Pluff, a plug-in framework for C
 * Copyright 2006 Johannes Lehtinen
 *-----------------------------------------------------------------------*/

/** @file
 * Plug-in scanning functionality
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
 * Function definitions
 * ----------------------------------------------------------------------*/

CP_C_API int cp_scan_plugins(cp_context_t *context, int flags) {
	hash_t *avail_plugins = NULL;
	list_t *started_plugins = NULL;
	cp_plugin_info_t **plugins = NULL;
	char *pdir_path = NULL;
	int pdir_path_size = 0;
	int plugins_stopped = 0;
	int status = CP_OK;
	
	CHECK_NOT_NULL(context);
	
	cpi_lock_context(context);
	cpi_check_invocation(context, CPI_CF_ANY, __func__);
	cpi_debug(context, "Plug-in scan is starting.");
	do {
		lnode_t *lnode;
		hscan_t hscan;
		hnode_t *hnode;
	
		// Create a hash for available plug-ins 
		if ((avail_plugins = hash_create(HASHCOUNT_T_MAX, (int (*)(const void *, const void *)) strcmp, NULL)) == NULL) {
			status = CP_ERR_RESOURCE;
			break;
		}
	
		// Scan plug-in directories for available plug-ins 
		lnode = list_first(context->env->plugin_dirs);
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
						int pdir_path_len = dir_path_len + 1 + strlen(de->d_name);
						cp_plugin_info_t *plugin;
						int s;
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
								cpi_errorf(context, _("Could not check possible plug-in location %s%c%s due to insufficient system resources."), dir_path, CP_FNAMESEP_CHAR, de->d_name);
								status = CP_ERR_RESOURCE;
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
						plugin = cp_load_plugin_descriptor(context, pdir_path, &s);
						if (plugin == NULL) {
							status = s;
							// continue loading plug-ins from other directories 
							continue;
						}
					
						// Insert plug-in to the list of available plug-ins 
						if ((hnode = hash_lookup(avail_plugins, plugin->identifier)) != NULL) {
							cp_plugin_info_t *plugin2 = hnode_get(hnode);
							if (plugin->if_version > plugin2->if_version) {
								hash_delete_free(avail_plugins, hnode);
								cpi_free_plugin(plugin2);
								hnode = NULL;
							}
						}
						if (hnode == NULL) {
							if (!hash_alloc_insert(avail_plugins, plugin->identifier, plugin)) {
								cpi_errorf(context, _("Plug-in %s version %s could not be loaded due to insufficient system resources."), plugin->identifier, plugin->release_version);
								cpi_free_plugin(plugin);
								status = CP_ERR_RESOURCE;
								// continue loading plug-ins from other directories 
								continue;
							}
						}
						
					}
					errno = 0;
				}
				if (errno) {
					cpi_errorf(context, _("Could not read plug-in directory %s: %s"), dir_path, strerror(errno));
					status = CP_ERR_IO;
					// continue loading plug-ins from other directories 
				}
				closedir(dir);
			} else {
				cpi_errorf(context, _("Could not open plug-in directory %s: %s"), dir_path, strerror(errno));
				status = CP_ERR_IO;
				// continue loading plug-ins from other directories 
			}
			
			lnode = list_next(context->env->plugin_dirs, lnode);
		}
		
		// Copy the list of started plug-ins, if necessary 
		if ((flags & CP_LP_RESTART_ACTIVE)
			&& (flags & (CP_LP_UPGRADE | CP_LP_STOP_ALL_ON_INSTALL))) {
			int s, i;

			if ((plugins = cp_get_plugins_info(context, &s, NULL)) == NULL) {
				status = s;
				break;
			}
			if ((started_plugins = list_create(LISTCOUNT_T_MAX)) == NULL) {
				status = CP_ERR_RESOURCE;
				break;
			}
			for (i = 0; plugins[i] != NULL; i++) {
				cp_plugin_state_t state;
				
				state = cp_get_plugin_state(context, plugins[i]->identifier);
				if (state == CP_PLUGIN_STARTING || state == CP_PLUGIN_ACTIVE) {
					char *pid;
				
					if ((pid = cpi_strdup(plugins[i]->identifier)) == NULL) {
						status = CP_ERR_RESOURCE;
						break;
					}
					if ((lnode = lnode_create(pid)) == NULL) {
						free(pid);
						status = CP_ERR_RESOURCE;
						break;
					}
					list_append(started_plugins, lnode);
				}
			}
			cp_release_info(plugins);
			plugins = NULL;
		}
		
		// Install/upgrade plug-ins 
		hash_scan_begin(&hscan, avail_plugins);
		while ((hnode = hash_scan_next(&hscan)) != NULL) {
			cp_plugin_info_t *plugin;
			cp_plugin_state_t state;
			int s;
			
			plugin = hnode_get(hnode);
			state = cp_get_plugin_state(context, plugin->identifier);
			
			// Unload the installed plug-in if it is to be upgraded 
			if (state >= CP_PLUGIN_INSTALLED
				&& (flags & CP_LP_UPGRADE)) {
				if ((flags & (CP_LP_STOP_ALL_ON_UPGRADE | CP_LP_STOP_ALL_ON_INSTALL))
					&& !plugins_stopped) {
					plugins_stopped = 1;
					cp_stop_plugins(context);
				}
				s = cp_uninstall_plugin(context, plugin->identifier);
				if (s != CP_OK) {
					status = s;
					break;
				}
				state = CP_PLUGIN_UNINSTALLED;
				assert(cp_get_plugin_state(context, plugin->identifier) == state);
			}
			
			// Install the plug-in, if to be installed 
			if (state == CP_PLUGIN_UNINSTALLED) {
				if ((flags & CP_LP_STOP_ALL_ON_INSTALL) && !plugins_stopped) {
					plugins_stopped = 1;
					cp_stop_plugins(context);
				}
				if ((s = cp_install_plugin(context, plugin)) != CP_OK) {
					status = s;
					break;
				}
				hash_scan_delfree(avail_plugins, hnode);
				cp_release_info(plugin);
			}
		}
		
		// Restart stopped plug-ins if necessary 
		if (started_plugins != NULL) {
			lnode = list_first(started_plugins);
			while (lnode != NULL) {
				char *pid;
				int s;
				
				pid = lnode_get(lnode);
				s = cp_start_plugin(context, pid);
				if (s != CP_OK) {
					status = s;
				}
				lnode = list_next(started_plugins, lnode);
			}
		}
		
	} while (0);
	cpi_unlock_context(context);

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
			cp_release_info(p);
		}
		hash_destroy(avail_plugins);
	}
	if (started_plugins != NULL) {
		list_process(started_plugins, NULL, cpi_process_free_ptr);
		list_destroy(started_plugins);
	}
	if (plugins != NULL) {
		cp_release_info(plugins);
	}

	// Error handling 
	switch (status) {
		case CP_OK:
			cpi_debug(context, "Plug-in scan has completed successfully.");
			break;
		case CP_ERR_RESOURCE:
			cpi_error(context, _("Could not scan plug-ins due to insufficient system resources."));
			break;
		default:
			cpi_error(context, _("Could not scan plug-ins."));
			break;
	}
	
	return status;
}
