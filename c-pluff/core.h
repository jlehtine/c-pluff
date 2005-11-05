/*-------------------------------------------------------------------------
 * C-Pluff, a plug-in framework for C
 * Copyright 2005 Johannes Lehtinen
 *-----------------------------------------------------------------------*/

/*
 * Core internal declarations
 */

#ifndef _CORE_H_
#define _CORE_H_

#include "cpluff.h"

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/


/* ------------------------------------------------------------------------
 * Function declarations
 * ----------------------------------------------------------------------*/

/**
 * Delivers a plug-in framework error to registered error handlers.
 * 
 * @param msg the error message
 */
void cpi_process_error(const char *msg);

/**
 * Delivers a plug-in event to registered event listeners.
 * 
 * @param event the event
 */
void cpi_deliver_event(const plugin_event_t *event);


#ifdef __cplusplus
}
#endif /*__cplusplus*/

#endif /*_CORE_H_*/
