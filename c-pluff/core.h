/*-------------------------------------------------------------------------
 * C-Pluff, a plug-in framework for C
 * Copyright 2005-2006 Johannes Lehtinen
 *-----------------------------------------------------------------------*/

/* TODO: Rename to context.h */

/*
 * Core internal declarations
 */

#ifndef CORE_H_
#define CORE_H_


/* ------------------------------------------------------------------------
 * Inclusions
 * ----------------------------------------------------------------------*/

#ifdef HAVE_GETTEXT
#include <libintl.h>
#endif
#include "defines.h"
#include "cpluff.h"
#include "kazlib/list.h"
#include "kazlib/hash.h"
#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif


#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/


/* ------------------------------------------------------------------------
 * Data types
 * ----------------------------------------------------------------------*/

/* Plug-in context */
struct cp_context_t {
	
	/** Mutex for accessing plug-in context */
	/*cpi_mutex_t mutex;*/
	
	/** Installed error handlers */
	list_t *error_handlers;
	
	/** Whether error handlers list is frozen */
	int error_handlers_frozen;

	/** Installed event listeners */
	list_t *event_listeners;
	
	/** Whether event listeners list is frozen */
	int event_listeners_frozen;

	/** Maps plug-in identifiers to plug-in state structures */
	hash_t *plugins;

	/** List of started plug-ins in the order they were started */
	list_t *started_plugins;
	
	/** Whether starting of plug-ins is denied */
	int plugin_starts_denied;

	/** Hash of in-use registered plug-ins by plugin pointer */
	hash_t *used_plugins;
	
};


/* ------------------------------------------------------------------------
 * Function declarations
 * ----------------------------------------------------------------------*/


/* Locking data structures for exclusive access */

/**
 * Acquires exclusive access to a plug-in context. This method must not be
 * called from a thread that already has acquired exclusive access but has
 * not released the data.
 * 
 * @param context the plug-in context
 */
void CP_LOCAL cpi_lock_context(cp_context_t *context);

/**
 * Releases exclusive access to a plug-in context.
 * 
 * @param context the plug-in context
 */
void CP_LOCAL cpi_unlock_context(cp_context_t *context);


/* Processing errors */

/**
 * Delivers a plug-in framework error to registered error handlers.
 * 
 * @param context the plug-in context
 * @param msg the error message
 */
void CP_LOCAL cpi_error(cp_context_t *context, const char *msg);

/**
 * Delivers a printf formatted plugin-in framework error to registered
 * error handlers.
 * 
 * @param context the plug-in context
 * @param msg the formatted error message
 * @param ... parameters
 */
void CP_LOCAL cpi_errorf(cp_context_t *context, const char *msg, ...) CP_PRINTF(2, 3);

/**
 * Delivers a plug-in framework error to the specified error handler.
 * 
 * @param context the plug-in context
 * @param error_handler the error handler or NULL if none
 * @param msg the error message
 */
void CP_LOCAL cpi_herror(cp_context_t *context, cp_error_handler_t error_handler, const char *msg);

/**
 * Delivers a printf formatted plug-in framework error to the specified
 * error handler.
 * 
 * @param context the plug-in context
 * @param error_handler the error handler or NULL if none
 * @param msg the formatted error message
 * @param ... parameters
 */
void CP_LOCAL cpi_herrorf(cp_context_t *context, cp_error_handler_t error_handler, const char *msg, ...)
	CP_PRINTF(3, 4);


/* Delivering plug-in events */

/**
 * Delivers a plug-in event to registered event listeners.
 * 
 * @param context the plug-in context
 * @param event the event
 */
void CP_LOCAL cpi_deliver_event(cp_context_t *context, const cp_plugin_event_t *event);


#ifdef __cplusplus
}
#endif /*__cplusplus*/

#endif /*CORE_H_*/
