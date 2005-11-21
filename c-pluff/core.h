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


/* Locking data structures for exclusive access */

/**
 * Acquires exclusive access to C-Pluff data structures. Access is granted to
 * the calling thread. This function does not block if the calling thread
 * already has exclusive access. If access is acquired multiple times by the
 * same thread then it is only released after corresponding number of calls to
 * release.
 */
void cpi_acquire_data(void);

/**
 * Releases exclusive access to C-Pluff data structures.
 */
void cpi_release_data(void);


/* Processing errors */

/**
 * Delivers a plug-in framework error to registered error handlers.
 * 
 * @param msg the error message
 */
void cpi_error(const char *msg);

/**
 * Delivers a printf formatted plugin-in framework error to registered
 * error handlers.
 * 
 * @param msg the formatted error message
 * @param ... parameters
 */
void cpi_errorf(const char *msg, ...);


/* Delivering plug-in events */

/**
 * Delivers a plug-in event to registered event listeners.
 * 
 * @param event the event
 */
void cpi_deliver_event(const cp_plugin_event_t *event);


#ifdef __cplusplus
}
#endif /*__cplusplus*/

#endif /*_CORE_H_*/
