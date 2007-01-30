/*-------------------------------------------------------------------------
 * C-Pluff, a plug-in framework for C
 * Copyright 2006 Johannes Lehtinen
 *-----------------------------------------------------------------------*/

/** @file
 * Internal data structures and declarations
 */

#ifndef INTERNAL_H_
#define INTERNAL_H_


/* ------------------------------------------------------------------------
 * Inclusions
 * ----------------------------------------------------------------------*/

#include "defines.h"
#if defined(DLOPEN_POSIX)
#include <dlfcn.h>
#elif defined(DLOPEN_LIBTOOL)
#include <ltdl.h>
#endif
#include "../kazlib/list.h"
#include "../kazlib/hash.h"
#include "cpluff.h"
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

/// Callback function logger function
#define CPI_CF_LOGGER 1

/// Callback function plug-in listener function
#define CPI_CF_LISTENER 2

/// Callback function start function
#define CPI_CF_START 4

/// Callback function stop function
#define CPI_CF_STOP 8

/// Bitmask corresponding to any callback function
#define CPI_CF_ANY (~0)


/* ------------------------------------------------------------------------
 * Macros
 * ----------------------------------------------------------------------*/

#if defined(DLOPEN_POSIX)
#define DLHANDLE void *
#define DLOPEN(name) dlopen((name), RTLD_LAZY | RTLD_GLOBAL)
#define DLSYM(handle, symbol) dlsym((handle), (symbol))
#define DLCLOSE(handle) dlclose(handle)
#elif defined(DLOPEN_LIBTOOL)
#define DLHANDLE lt_dlhandle
#define DLOPEN(name) lt_dlopen(name)
#define DLSYM(handle, symbol) lt_dlsym((handle), (symbol))
#define DLCLOSE(handle) lt_dlclose(handle)
#endif


/**
 * Checks that the specified function argument is not NULL.
 * Otherwise, reports a fatal error.
 * 
 * @param arg the argument
 */
#define CHECK_NOT_NULL(arg) do { if ((arg) == NULL) cpi_fatal_null_arg(#arg, __func__); } while (0)


/* ------------------------------------------------------------------------
 * Data types
 * ----------------------------------------------------------------------*/

typedef struct cp_plugin_t cp_plugin_t;
typedef struct cp_plugin_env_t cp_plugin_env_t;

// Plug-in context
struct cp_context_t {
	
	/// The associated plug-in instance or NULL for the main program
	cp_plugin_t *plugin;
	
	/// The associated plug-in environment
	cp_plugin_env_t *env;

	/// Information about resolved symbols or NULL if not initialized
	hash_t *resolved_symbols;

	/// Information about symbol providing plugins or NULL if not initialized
	hash_t *symbol_providers;
	
};

// Plug-in environment
struct cp_plugin_env_t {

#if defined(CP_THREADS)

	/// Mutex for accessing this plug-in environment.
	/// This mutex is signaled when a run function returns.
	cpi_mutex_t *mutex;
	
#elif !defined(NDEBUG)
	int locked;
#endif

	/// Number of startup arguments
	int argc;
	
	/// An array of startup arguments
	char **argv;
	
	/// Installed plug-in listeners 
	list_t *plugin_listeners;
	
	/// List of registered plug-in directories 
	list_t *plugin_dirs;

	/// Maps plug-in identifiers to plug-in state structures 
	hash_t *plugins;

	/// List of started plug-ins in the order they were started 
	list_t *started_plugins;

	/// Maps extension point names to installed extension points
	hash_t *ext_points;
	
	/// Maps extension point names to installed extensions
	hash_t *extensions;
	
	/// FIFO queue of run functions, currently running functions at front
	list_t *run_funcs;
	
	/// First waiting run function, or NULL if none
	lnode_t *run_wait;

	/// Whether currently in event listener invocation
	int in_event_listener_invocation;
	
	// Whether currently in start function invocation
	int in_start_func_invocation;
	
	// Whether currently in stop function invocation
	int in_stop_func_invocation;
	
	// Whether currently in create function invocation
	int in_create_func_invocation;
	
	// Whether currently in destroy function invocation
	int in_destroy_func_invocation;
	
};

// Plug-in instance
struct cp_plugin_t {
	
	/// The enclosing context or NULL if none exists
	cp_context_t *context;
	
	/// Plug-in information 
	cp_plugin_info_t *plugin;
	
	/// The current state of the plug-in 
	cp_plugin_state_t state;
	
	/// The set of imported plug-ins, or NULL if not resolved 
	list_t *imported;
	
	/// The set of plug-ins importing this plug-in 
	list_t *importing;
	
	/// The runtime library handle, or NULL if not resolved 
	DLHANDLE runtime_lib;
	
	/// Plug-in runtime function information, or NULL if not resolved
	cp_plugin_runtime_t *runtime_funcs;

	/// Plug-in instance data or NULL if instance does not exist
	void *plugin_data;
	
	/// Context specific symbols defined by the plug-in
	hash_t *defined_symbols;
	
	/// The usage count for the symbols of this plug-in
	int syms_usage_count;

	/// Used by recursive operations: has this plug-in been processed already
	int processed;
	
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
CP_HIDDEN void cpi_lock_framework(void);

/**
 * Releases exclusive access to the framework.
 */
CP_HIDDEN void cpi_unlock_framework(void);

/**
 * Acquires exclusive access to a plug-in context and the associated
 * plug-in environment.
 * 
 * @param context the plug-in context
 */
CP_HIDDEN void cpi_lock_context(cp_context_t *context) CP_NONNULL(1);

/**
 * Releases exclusive access to a plug-in context.
 * 
 * @param context the plug-in context
 */
CP_HIDDEN void cpi_unlock_context(cp_context_t *context) CP_NONNULL(1);

/**
 * Waits until the specified plug-in context is signalled.
 * 
 * @param context the plug-in context
 */
CP_HIDDEN void cpi_wait_context(cp_context_t *context) CP_NONNULL(1);

/**
 * Signals the specified plug-in context.
 * 
 * @param context the plug-in context
 */
CP_HIDDEN void cpi_signal_context(cp_context_t *context) CP_NONNULL(1);

#else
#define cpi_lock_context(dummy) do {} while (0)
#define cpi_unlock_context(dummy) do {} while (0)
#define cpi_wait_context(dummy) do {} while (0)
#define cpi_signal_context(dummy) do {} while (0)
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
CP_HIDDEN void cpi_log(cp_context_t *ctx, cp_log_severity_t severity, const char *msg) CP_NONNULL(3);

/**
 * Formats and logs a message.
 * 
 * @param ctx the related plug-in context or NULL if none
 * @param severity the severity of the message
 * @param msg the localized message format
 * @param ... the message parameters
 */
CP_HIDDEN CP_PRINTF(3, 4) void cpi_logf(cp_context_t *ctx, cp_log_severity_t severity, const char *msg, ...) CP_NONNULL(3);

/**
 * Returns whether the messages of the specified severity level are
 * being logged.
 * 
 * @param severity the severity
 * @return whether the messages of the specified severity level are logged
 */
CP_HIDDEN int cpi_is_logged(cp_log_severity_t severity);

// Convenience macros for logging
#define cpi_error(ctx, msg) cpi_log((ctx), CP_LOG_ERROR, (msg))
#define cpi_errorf(ctx, msg, ...) cpi_logf((ctx), CP_LOG_ERROR, (msg), __VA_ARGS__)
#define cpi_warn(ctx, msg) cpi_log((ctx), CP_LOG_WARNING, (msg))
#define cpi_warnf(ctx, msg, ...) cpi_logf((ctx), CP_LOG_WARNING, (msg), __VA_ARGS__)
#define cpi_info(ctx, msg) cpi_log((ctx), CP_LOG_INFO, (msg))
#define cpi_infof(ctx, msg, ...) cpi_logf((ctx), CP_LOG_INFO, (msg), __VA_ARGS__)
#ifndef NDEBUG
#define cpi_debug(ctx, msg) cpi_log((ctx), CP_LOG_DEBUG, (msg))
#define cpi_debugf(ctx, msg, ...) cpi_logf((ctx), CP_LOG_DEBUG, (msg), __VA_ARGS__)
#else
#define cpi_debug(ctx, msg) do {} while (0)
#define cpi_debugf(ctx, msg, ...) do {} while (0)
#endif

#ifndef NDEBUG

/**
 * Returns the owner name for a context.
 * 
 * @param ctx the context
 * @return owner name
 */
CP_HIDDEN const char *cpi_context_owner(cp_context_t *ctx) CP_NONNULL(1);

#endif

/**
 * Reports a fatal error. This method does not return.
 * 
 * @param msg the formatted error message
 * @param ... parameters
 */
CP_HIDDEN CP_NORETURN void cpi_fatalf(const char *msg, ...) CP_PRINTF(1, 2) CP_NONNULL(1);

/**
 * Reports a fatal NULL argument to an API function.
 * 
 * @param arg the argument name
 * @param func the API function name
 */
CP_HIDDEN CP_NORETURN void cpi_fatal_null_arg(const char *arg, const char *func) CP_NONNULL(1, 2);

/**
 * Checks that we are currently not in a specific callback function invocation.
 * Otherwise, reports a fatal error. If no context is specified then only
 * logger function invocations are checked. If context is specified then the
 * caller must have locked the context before calling this function.
 * 
 * @param ctx the associated plug-in context or NULL if none
 * @param funcmask the bitmask of disallowed callback functions
 * @param func the current plug-in framework function
 */
CP_HIDDEN void cpi_check_invocation(cp_context_t *ctx, int funcmask, const char *func) CP_NONNULL(3);


// Context management

/**
 * Allocates a new plug-in context.
 * 
 * @param plugin the associated plug-in or NULL for the client program
 * @param env the associated plug-in environment
 * @param status a pointer to the location where the status code is to be stored
 * @return the newly allocated context or NULL on failure
 */
CP_HIDDEN cp_context_t * cpi_new_context(cp_plugin_t *plugin, cp_plugin_env_t *env, cp_status_t *status) CP_NONNULL(2, 3);

/**
 * Frees the resources associated with a plug-in context. Also frees the
 * associated plug-in environment if the context is a client program plug-in
 * context.
 * 
 * @param context the plug-in context to free
 */
CP_HIDDEN void cpi_free_context(cp_context_t *context) CP_NONNULL(1);

/**
 * Destroys all contexts and releases the context list resources.
 */
CP_HIDDEN void cpi_destroy_all_contexts(void);


// Delivering plug-in events 

/**
 * Delivers a plug-in event to registered event listeners.
 * 
 * @param context the plug-in context
 * @param event the plug-in event
 */
CP_HIDDEN void cpi_deliver_event(cp_context_t *context, const cpi_plugin_event_t *event) CP_NONNULL(1, 2);


// Plug-in management

/**
 * Frees any resources allocated for a plug-in description.
 * 
 * @param plugin the plug-in to be freed
 */
CP_HIDDEN void cpi_free_plugin(cp_plugin_info_t *plugin) CP_NONNULL(1);

/**
 * Starts the specified plug-in and its dependencies.
 * 
 * @param context the plug-in context
 * @param plugin the plug-in
 * @return @ref CP_OK (zero) on success or an error code on failure
 */
CP_HIDDEN cp_status_t cpi_start_plugin(cp_context_t *context, cp_plugin_t *plugin) CP_NONNULL(1, 2);


// Dynamic resource management

/**
 * Registers a new dynamic information object which uses reference counting.
 * Initializes the use count to 1.
 * 
 * @param res the resource
 * @param df the deallocation function
 * @return @ref CP_OK (zero) on success or an error code on failure
 */
CP_HIDDEN cp_status_t cpi_register_info(void *res, cpi_dealloc_func_t df) CP_NONNULL(1, 2);

/**
 * Increases the usage count for the specified dynamic information object.
 * 
 * @param res the resource
 */
CP_HIDDEN void cpi_use_info(void *res) CP_NONNULL(1);

/**
 * Destroys all dynamic information objects.
 */
CP_HIDDEN void cpi_destroy_all_infos(void);


// Serialized execution

/**
 * Waits for all the run functions registered by the specified plug-in to
 * return and then unregisters them.
 * 
 * @param plugin the plug-in to be stopped
 */
CP_HIDDEN void cpi_stop_plugin_run(cp_plugin_t *plugin) CP_NONNULL(1);


#ifdef __cplusplus
}
#endif //__cplusplus

#endif /*INTERNAL_H_*/
