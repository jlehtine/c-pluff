/*-------------------------------------------------------------------------
 * C-Pluff, a plug-in framework for C
 * Copyright 2005-2006 Johannes Lehtinen
 *-----------------------------------------------------------------------*/

#ifndef _PLUGIN_H_
#define _PLUGIN_H_

#include "context.h"

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/


/* ------------------------------------------------------------------------
 * Constants
 * ----------------------------------------------------------------------*/

/** Preliminarily OK */
#define CP_OK_PRELIMINARY 1


/* ------------------------------------------------------------------------
 * Function declarations
 * ----------------------------------------------------------------------*/

/**
 * Installs a plug-in.
 * 
 * @param context the plug-in context
 * @param plugin the plug-in to be installed
 * @param usage the initial usage count for plug-in information structure
 * @return CP_OK (0) on success, an error code on failure
 */
int CP_LOCAL cpi_install_plugin(cp_context_t *context, cp_plugin_t *plugin, unsigned int use_count);

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
