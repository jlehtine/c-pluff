/*-------------------------------------------------------------------------
 * C-Pluff, a plug-in framework for C
 * Copyright 2005-2006 Johannes Lehtinen
 *-----------------------------------------------------------------------*/

#ifndef _PLUGIN_H_
#define _PLUGIN_H_

#include "core.h"

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/


/* ------------------------------------------------------------------------
 * Constants
 * ----------------------------------------------------------------------*/

/** Preliminarily OK */
#define CP_OK_PRELIMINARY 1


/* ------------------------------------------------------------------------
 * Data types
 * ----------------------------------------------------------------------*/



/* ------------------------------------------------------------------------
 * Function declarations
 * ----------------------------------------------------------------------*/

/**
 * Initializes the plug-in controlling component.
 * 
 * @return CP_OK (0) on success, an error code on failure
 */
int CP_LOCAL cpi_init_plugins(void);

/**
 * Destroys the plug-in controlling component.
 */
void CP_LOCAL cpi_destroy_plugins(void);

/**
 * Installs a plug-in.
 * 
 * @param plugin the plug-in to be installed
 * @return CP_OK (0) on success, an error code on failure
 */
int CP_LOCAL cpi_install_plugin(cp_plugin_t *plugin);

/**
 * Frees any resources allocated for a plug-in description.
 * 
 * @param plugin the plug-in to be freed
 */
void CP_LOCAL cpi_free_plugin(cp_plugin_t *plugin);

/**
 * Frees any resources allocated for a configuration element.
 * 
 * @param cfg_element the configuration element to be freed
 */
void CP_LOCAL cpi_free_cfg_element(cp_cfg_element_t *cfg_element);


#ifdef __cplusplus
}
#endif /*__cplusplus */

#endif /*_PLUGIN_H_*/
