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
#define CP_OK_PRELIMINARY (-1)


/* ------------------------------------------------------------------------
 * Macros
 * ----------------------------------------------------------------------*/

#ifndef NDEBUG
#define cpi_inc_error_invocation(context) ((context)->in_error_handler_invocation++)
#define cpi_dec_error_invocation(context) ((context)->in_error_handler_invocation--)
#define cpi_inc_event_invocation(context) ((context)->in_event_listener_invocation++)
#define cpi_dev_event_invocation(context) ((context)->in_event_listener_invocation--)
#define cpi_inc_start_invocation(context) ((context)->in_start_func_invocation++)
#define cpi_dec_start_invocation(context) ((context)->in_start_func_invocation--)
#define cpi_inc_stop_invocation(context) ((context)->in_stop_func_invocation++)
#define cpi_dec_stop_invocation(context) ((context)->in_stop_func_invocation--)
#else
#define cpi_inc_error_invocation(context) do {} while (0)
#define cpi_dec_error_invocation(context) do {} while (0)
#define cpi_inc_event_invocation(context) do {} while (0)
#define cpi_dev_event_invocation(context) do {} while (0)
#define cpi_inc_start_invocation(context) do {} while (0)
#define cpi_dec_start_invocation(context) do {} while (0)
#define cpi_inc_stop_invocation(context) do {} while (0)
#define cpi_dec_stop_invocation(context) do {} while (0)
#endif


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
	
	/// Installed plug-in listeners 
	list_t *plugin_listeners;
	
	/// List of registered plug-in directories 
	list_t *plugin_dirs;

	/// Maps plug-in identifiers to plug-in state structures 
	hash_t *plugins;

	/// List of started plug-ins in the order they were started 
	list_t *started_plugins;

#ifndef NDEBUG

	/// Whether currently in error handler invocation 
	int in_error_handler_invocation;
	
	/// Whether currently in event listener invocation
	int in_event_listener_invocation;
	
	// Whether currently in start function invocation
	int in_start_func_invocation;
	
	// Whether currently in stop function invocation
	int in_stop_func_invocation;
	
#endif

};

/// Resource deallocation function
typedef void (*cpi_dealloc_func_t)(void *resource);

typedef struct cpi_plugin_event_t cpi_plugin_event_t;

/// Plug-in event information
struct cpi_plugin_event_t {
	
	/// The affect plug-in
	const char *plugin_id;
	
	/// Old state
	cp_plugin_state_t old_state;
	
	/// New state
	cp_plugin_state_t new_state;
};

/* ------------------------------------------------------------------------
 * Function declarations
 * ----------------------------------------------------------------------*/


// Locking data structures for exclusive access 

#if defined(CP_THREADS) || !defined(NDEBUG)

/**
 * Acquires exclusive access to the framework. Thread having the framework
 * lock must not acquire plug-in context lock (it is ok to retain a previously
 * acquired plug-in context lock).
 */
void CP_LOCAL cpi_lock_framework(void);

/**
 * Releases exclusive access to the framework.
 */
void CP_LOCAL cpi_unlock_framework(void);

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
#define cpi_lock_framework() do {} while(0)
#define cpi_unlock_framework() do {} while(0)
#endif


// Logging

/**
 * Logs a message.
 * 
 * @param ctx the related plug-in context or NULL if none
 * @param severity the severity of the message
 * @param msg the localized message
 */
void CP_LOCAL cpi_log(cp_context_t *ctx, cp_log_severity_t severity, const char *msg);

/**
 * Formats and logs a message.
 * 
 * @param ctx the related plug-in context or NULL if none
 * @param severity the severity of the message
 * @param msg the localized message format
 * @param ... the message parameters
 */
void CP_LOCAL cpi_logf(cp_context_t *ctx, cp_log_severity_t severity, const char *msg, ...) CP_PRINTF(3, 4);

/**
 * Returns whether the messages of the specified severity level are
 * being logged.
 * 
 * @param severity the severity
 * @return whether the messages of the specified severity level are logged
 */
int CP_LOCAL cpi_is_logged(cp_log_severity_t severity);

// Convenience macros for logging
#define cpi_error(ctx, msg) cpi_log((ctx), CP_LOG_ERROR, (msg))
#define cpi_errorf(ctx, msg, ...) cpi_logf((ctx), CP_LOG_ERROR, (msg), __VA_ARGS__)
#define cpi_warn(ctx, msg) cpi_log((ctx), CP_LOG_WARNING, (msg))
#define cpi_warnf(ctx, msg, ...) cpi_logf((ctx), CP_LOG_WARNING, (msg), __VA_ARGS__)
#define cpi_info(ctx, msg) cpi_log((ctx), CP_LOG_INFO, (msg))
#define cpi_infof(ctx, msg, ...) cpi_logf((ctx), CP_LOG_INFO, (msg), __VA_ARGS__)
#define cpi_debug(ctx, msg) cpi_log((ctx), CP_LOG_DEBUG, (msg))
#define cpi_debugf(ctx, msg, ...) cpi_logf((ctx), CP_LOG_DEBUG, (msg), __VA_ARGS__)

/**
 * Reports a fatal error. This method does not return.
 * 
 * @param msg the formatted error message
 * @param ... parameters
 */
void CP_LOCAL cpi_fatalf(const char *msg, ...) CP_PRINTF(1, 2) CP_NORETURN;

#ifndef NDEBUG

/**
 * Checks that we are currently not in an error handler, event listener,
 * start function or stop function invocation. Otherwise, reports a fatal
 * error.
 * 
 * @param ctx the associated plug-in context
 * @param func the current plug-in framework function
 */
void CP_LOCAL cpi_check_invocation(cp_context_t *ctx, const char *func);

#else
#define cpi_check_invocation(a, b) do {} while (0)
#endif


// Context management

/**
 * Destroys all contexts and releases the context list resources.
 */
void CP_LOCAL cpi_destroy_all_contexts(void);


// Delivering plug-in events 

/**
 * Delivers a plug-in event to registered event listeners.
 * 
 * @param context the plug-in context
 * @param event the plug-in event
 */
void CP_LOCAL cpi_deliver_event(cp_context_t *context, const cpi_plugin_event_t *event);


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
