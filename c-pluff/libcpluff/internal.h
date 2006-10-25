/*-------------------------------------------------------------------------
 * C-Pluff, a plug-in framework for C
 * Copyright 2006 Johannes Lehtinen
 *-----------------------------------------------------------------------*/

/*
 * C-Pluff internal declarations
 */

#ifndef INTERNAL_H_
#define INTERNAL_H_


/* ------------------------------------------------------------------------
 * Inclusions
 * ----------------------------------------------------------------------*/

#include "../kazlib/list.h"
#include "../kazlib/hash.h"
#include "cpluff.h"
#include "defines.h"
#ifdef CP_THREADS
#include "thread.h"
#endif


#ifdef __cplusplus
extern "C" {
#endif //__cplusplus


/* ------------------------------------------------------------------------
 * Constants
 * ----------------------------------------------------------------------*/

/// Preliminarily OK 
#define CP_OK_PRELIMINARY 1


/* ------------------------------------------------------------------------
 * Data types
 * ----------------------------------------------------------------------*/

// Plug-in context 
struct cp_context_t {

#if defined(CP_THREADS)

	/// Mutex for accessing plug-in context 
	cpi_mutex_t *mutex;
	
#elif !defined(NDEBUG)
	int locked;
#endif
	
	/// Installed error handlers 
	list_t *error_handlers;
	
	/// Whether error handlers list is frozen 
	int error_handlers_frozen;

	/// Installed event listeners 
	list_t *event_listeners;
	
	/// Whether event listeners list is frozen 
	int event_listeners_frozen;
	
	/// List of registered plug-in directories 
	list_t *plugin_dirs;

	/// Maps plug-in identifiers to plug-in state structures 
	hash_t *plugins;

	/// List of started plug-ins in the order they were started 
	list_t *started_plugins;
	
};

/// Resource deallocation function
typedef void (*cpi_dealloc_func_t)(void *resource);


/* ------------------------------------------------------------------------
 * Function declarations
 * ----------------------------------------------------------------------*/


// Locking data structures for exclusive access 

#if defined(CP_THREADS) || !defined(NDEBUG)

/**
 * Acquires exclusive access to a plug-in context.
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

#else
#define cpi_lock_context(dummy) do {} while (0)
#define cpi_unlock_context(dummy) do {} while (0)
#endif


// Error handling 

/**
 * Reports a fatal error. This method does not return.
 * 
 * @param msg the formatted error message
 * @param ... parameters
 */
void CP_LOCAL cpi_fatalf(const char *msg, ...) CP_PRINTF(1, 2) CP_NORETURN;

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
 * @param user_data the user data pointer supplied at registration
 * @param msg the error message
 */
void CP_LOCAL cpi_herror(cp_context_t *context, cp_error_handler_t error_handler, void *user_data, const char *msg);

/**
 * Delivers a printf formatted plug-in framework error to the specified
 * error handler.
 * 
 * @param context the plug-in context
 * @param error_handler the error handler or NULL if none
 * @param user_data the user data pointer supplied at registration
 * @param msg the formatted error message
 * @param ... parameters
 */
void CP_LOCAL cpi_herrorf(cp_context_t *context, cp_error_handler_t error_handler, void *user_data, const char *msg, ...)
	CP_PRINTF(4, 5);


// Delivering plug-in events 

/**
 * Delivers a plug-in event to registered event listeners.
 * 
 * @param context the plug-in context
 * @param event the event
 */
void CP_LOCAL cpi_deliver_event(cp_context_t *context, const cp_plugin_event_t *event);


// Plug-in descriptor management

/**
 * Frees any resources allocated for a plug-in description.
 * 
 * @param plugin the plug-in to be freed
 */
void CP_LOCAL cpi_free_plugin(cp_plugin_info_t *plugin);


// Dynamic resource management

/**
 * Registers a new dynamic resource which uses reference counting.
 * Initializes the use count to 1.
 * 
 * @param res the resource
 * @param df the deallocation function
 * @return CP_OK (zero) on success or error code on failure
 */
int CP_LOCAL cpi_register_resource(void *res, cpi_dealloc_func_t df);

/**
 * Increases the usage count for the specified dynamically allocated
 * registered resource.
 * 
 * @param res the resource
 */
void CP_LOCAL cpi_use_resource(void *res);


#ifdef __cplusplus
}
#endif //__cplusplus

#endif /*INTERNAL_H_*/
