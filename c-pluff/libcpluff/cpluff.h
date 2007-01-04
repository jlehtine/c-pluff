/*-------------------------------------------------------------------------
 * C-Pluff, a plug-in framework for C
 * Copyright 2006 Johannes Lehtinen
 *-----------------------------------------------------------------------*/

/*
 * The public C API header file
 */

#ifndef CPLUFF_H_
#define CPLUFF_H_

/** @name C-Pluff C API */
/*@{*/

#include "cpluffdef.h"
/*@Include: cpluffdef.h*/

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/


/* ------------------------------------------------------------------------
 * Data types
 * ----------------------------------------------------------------------*/

/** @name Data types */
/*@{*/


/** @name Opaque types */
/*@{*/

/**
 * A plug-in context represents the co-operation environment of a set of
 * plug-ins from the perspective of a particular participating plug-in or
 * the perspective of the client program. It is used as an opaque handle to
 * the shared resources but the framework also uses the context to identify
 * the plug-in or the client program invoking framework functions. Therefore
 * a plug-in should not generally expose its context instance to other
 * plug-ins or the client program and neither should the client program
 * expose its context instance to plug-ins.
 */
typedef struct cp_context_t cp_context_t;

/*@}*/


/* Forward type definitions */
typedef struct cp_core_info_t cp_core_info_t;
typedef struct cp_plugin_import_t cp_plugin_import_t;
typedef struct cp_ext_point_t cp_ext_point_t;
typedef struct cp_cfg_element_t cp_cfg_element_t;
typedef struct cp_extension_t cp_extension_t;
typedef struct cp_plugin_info_t cp_plugin_info_t;
typedef struct cp_plugin_runtime_t cp_plugin_runtime_t;


/** @name Library information */
/*@{*/

/**
 * Implementation information structure contains information about
 * the C-Pluff core library implementation.
 *
 * @see cp_get_core_info
 */
struct cp_core_info_t {

	/** The C-Pluff release version */
	char *release_version;
	
	/** The C-Pluff core library version */
	char *core_version;
	
	/** The version of the core library API */
	int core_api_version;
	
	/** The revision of the core library API */
	int core_api_revision;
	
	/** The backwards compatibility age of the core library API */
	int core_api_age;
  
	/** The canonical host type */
	char *host_type;
	
	/** The type of multi-threading support, or NULL for none */
	char *multi_threading_type;
	
};

/*@}*/


/** @name Plug-in information */
/*@{*/

/**
 * Plug-in structure captures information about a plug-in.
 *
 * @see cp_get_plugin_info
 * @see cp_get_plugin_infos
 */
struct cp_plugin_info_t {
	
	/** Human-readable, possibly localized plug-in name */
	char *name;
	
	/** Unique identifier */
	char *identifier;
	
	/** Version string */
	char *version;
	
	/** Human-readable, possibly localized provider name */
	char *provider_name;
	
	/** Path of the plugin directory, or NULL if not known */
	char *plugin_path;
	
	/** Number of imports */
	unsigned int num_imports;
	
	/** Imports */
	cp_plugin_import_t *imports;

    /**
     * The plug-in runtime library path, relative to the plug-in directory,
     * or NULL if none
     */
    char *lib_path;
    
    /** The symbol pointing to the plug-in runtime function information or NULL if none */
    char *runtime_funcs_symbol;
    
	/** Number of extension points provided by this plug-in */
	unsigned int num_ext_points;
	
	/** Extension points provided by this plug-in */
	cp_ext_point_t *ext_points;
	
	/** Number of extensions provided by this plugin */
	unsigned int num_extensions;
	
	/** Extensions provided by this plug-in */
	cp_extension_t *extensions;

};

/**
 * Enumeration of possible version matching criteria.
 *
 * @see cp_plugin_import_t
 */
typedef enum cp_version_match_t {

	/** No version criteria for the dependency */
	CP_MATCH_NONE,
	
	/** Perfect version match is required */
	CP_MATCH_PERFECT,
	
	/**
	 * Equivalent version is required. An equivalent version has the same
	 * major and minor version and the same or more recent revision.
	 */
	CP_MATCH_EQUIVALENT,
	
	/**
	 * Compatible version is required. A compatible version has the same
	 * major version number and the same or more recent minor version.
	 */
	CP_MATCH_COMPATIBLE,
	
	/** Same or more recent version is required */
	CP_MATCH_GREATEROREQUAL
	
} cp_version_match_t;

/**
 * Information about plug-in import.
 *
 * @see cp_plugin_info_t
 */
struct cp_plugin_import_t {
	
	/**
	 * Identifier of the imported plug-in. The reserved identifier
	 * "org.cpluff.core" can be used to define a versioned dependency to the
	 * core framework library.
	 */
	char *plugin_id;
	
	/** Version to be matched, or NULL if none */
	char *version;
	
	/** Version matching rule */
	cp_version_match_t match;
	
	/** Whether this import is optional (1 for optional, 0 for mandatory) */
	int optional;
};

/**
 * Extension point structure captures information about an extension
 * point.
 */
struct cp_ext_point_t {

	/** The providing plug-in */
	cp_plugin_info_t *plugin;
	
	/**
	 * Human-readable, possibly localized extension point name or NULL
	 * if not available
	 */
	char *name;
	
	/**
	 * Local identifier uniquely identifying the extension point within the
	 * providing plug-in
	 */
	char *local_id;
	
	/** Unique identifier of the extension point */
	char *global_id;

	/**
	 * Path to the extension schema definition (relative to the plug-in directory)
	 * or NULL if none.
	 */
	char *schema_path;
};

/**
 * Extension structure captures information about an extension.
 */
struct cp_extension_t {

	/** The providing plug-in */
	cp_plugin_info_t *plugin;
	
	/** 
	 * Human-readable, possibly localized, extension name or NULL if not
	 * available
	 **/
	char *name;
	
	/**
	 * Local identifier uniquely identifying the extension within the
	 * providing plug-in or NULL if not available
	 */
	char *local_id;

    /** Unique identifier of the extension or NULL if not available */
    char *global_id;
	 
	/** Unique identifier of the extension point */
	char *ext_point_id;
	
	/** Extension configuration (starting with the extension element) */
	cp_cfg_element_t *configuration;
};

/**
 * Configuration element contains configuration information for an extension.
 */
struct cp_cfg_element_t {
	
	/** Name of the configuration element */
	char *name;

	/** Number of attributes */
	unsigned int num_atts;
	
	/**
	 * Attribute name, value pairs (alternating, values possibly localized)
	 */
	char **atts;
	
	/**
	  * The value (text contents) of this configuration element,
	  * possibly localized, or NULL if none
	  */
	char *value;
	
	/** The parent element, or NULL if extension element */
 	cp_cfg_element_t *parent;
 	
 	/** The index among siblings (0-based) */
 	unsigned int index;
 	
 	/** Number of children */
 	unsigned int num_children;

	/** Children */
	cp_cfg_element_t *children;
};

/*@}*/


/** @name Plug-in runtime */
/*@{*/

/**
 * A plug-in runtime structure containing pointers to plug-in
 * control functions. A plug-in runtime defines a static instance of this
 * structure to pass information about the available control functions
 * to the plug-in framework. The plug-in framework then uses the
 * functions to create and control plug-in instances.
 */
struct cp_plugin_runtime_t {

	/**
	 * An initialization function called to create a new plug-in
	 * runtime instance. The initialization function initializes and
	 * returns an opaque plug-in instance data pointer which is then
	 * passed on to other control functions. This data pointer should
	 * be used to access plug-in instance specific data. For example,
	 * the context reference must be stored as part of plug-in instance
	 * data if the plug-in runtime needs it. On failure, the function
	 * must return NULL. Plug-in framework functions must not be called
	 * from within a create function invocation.
	 * 
	 * @param ctx the plug-in context of the new plug-in instance
	 * @return an opaque pointer to plug-in instance data or NULL on failure
	 */  
	void *(*create)(cp_context_t *ctx);

	/**
	 * A start function called to start a plug-in instance.
	 * The start function must return zero (CP_OK) on success and non-zero
	 * on failure. If the start fails then the stop function (if any) is
	 * called to clean up plug-in state. Library initialization, plug-in
	 * context management and plug-in management functions must not be
	 * called from within a start function invocation. The start function
	 * pointer can be NULL if the plug-in runtime does not have a start
	 * function.
	 * 
	 * @param data an opaque pointer to plug-in instance data
	 * @return non-zero on success, or zero on failure
	 */
	int (*start)(void *data);
	
	/**
	 * A stop function called to stop a plugin instance. Library
	 * initialization, plug-in context management, plug-in management
	 * functions and cp_resolve_symbol must not be called from within
	 * a stop function invocation. The stop function pointer can
	 * be NULL if the plug-in runtime does not have a stop function.
	 *
	 * @param data an opaque pointer to plug-in instance data
	 */
	void (*stop)(void *data);

	/**
 	 * A destroy function called to destroy a plug-in instance.
 	 * This function should release any plug-in instance data.
 	 * The plug-in is stopped before this function is called.
 	 *  Plug-in framework functions must not be called
	 * from within a destroy function invocation.
	 *
	 * @param data an opaque pointer to plug-in instance data
	 */
	void (*destroy)(void *data);

};

/*@}*/


/** @name Plug-in state */
/*@{*/

/**
 * An enumeration of possible plug-in states.
 *
 * @see cp_plugin_listener_t
 * @see cp_get_plugin_state
 */
typedef enum cp_plugin_state_t {

	/**
	 * Plug-in is not installed. No plug-in information has been
	 * loaded.
	 */
	CP_PLUGIN_UNINSTALLED,
	
	/**
	 * Plug-in is installed. At this stage the plug-in information has
	 * been loaded but its dependencies to other plug-ins has not yet
	 * been resolved. The plug-in runtime has not been loaded yet.
	 * The extension points and extensions provided by the plug-in
	 * have been registered.
	 */
	CP_PLUGIN_INSTALLED,
	
	/**
	 * Plug-in dependencies have been resolved. At this stage it has
	 * been verified that the dependencies of the plug-in are satisfied
	 * and the plug-in runtime has been loaded but it is not active
	 * (it has not been started or it has been stopped).
	 * Plug-in is resolved when a dependent plug-in is being
	 * resolved or before the plug-in is started. Plug-in is put
	 * back to installed stage if its dependencies are being
	 * uninstalled.
	 */
	CP_PLUGIN_RESOLVED,
	
	/**
	 * Plug-in is starting. The plug-in has been resolved and the start
	 * function (if any) of the plug-in runtime is about to be called.
	 * A plug-in is started when explicitly requested by the client
	 * program or when a dependent plug-in is about to be started or when
	 * a dynamic symbol defined by the plug-in is being resolved. This state
	 * is omitted and the state changes directly from resolved to active
	 * if the plug-in runtime does not define a start function.
	 */
	CP_PLUGIN_STARTING,
	
	/**
	 * Plug-in is stopping. The stop function (if any) of the plug-in
	 * runtime is about to be called. A plug-in is stopped if the start
	 * function fails or when stopping is explicitly
	 * requested by the client program or when its dependencies are being
	 * stopped. This state is omitted and the state changes directly from
	 * active to resolved if the plug-in runtime does not define a stop
	 * function.
	 */
	CP_PLUGIN_STOPPING,
	
	/**
	 * Plug-in has been successfully started and it has not yet been
	 * stopped.
	 */
	CP_PLUGIN_ACTIVE
	
} cp_plugin_state_t;

/**
 * A listener function called synchronously after a plugin state change.
 * The function should return promptly. Library initialization, plug-in context
 * management, plug-in management, plug-in listener registration and dynamic
 * symbol functions must not be called from within a plug-in listener
 * invocation.
 * 
 * @param plugin_id the plug-in identifier
 * @param old_state the old plug-in state
 * @param new_state the new plug-in state
 * @param user_data the user data pointer supplied at listener registration
 * @see cp_add_plugin_listener
 */
typedef void (*cp_plugin_listener_func_t)(const char *plugin_id, cp_plugin_state_t old_state, cp_plugin_state_t new_state, void *user_data);

/*@}*/


/** @name Logging */
/*@{*/

/**
 * An enumeration of possible message severities for logging.
 */
typedef enum cp_log_severity_t {

	/**
	 * Used for detailed debug messages. This level of logging is enabled
	 * only if debugging has been enabled at framework compilation time.
	 */
	CP_LOG_DEBUG,
	
	/** Used for informational messages such as plug-in state changes */
	CP_LOG_INFO,
	
	/** Used for messages warning about possible problems */
	CP_LOG_WARNING,
	
	/** Used for messages reporting plug-in framework errors */
	CP_LOG_ERROR
	
} cp_log_severity_t;

/**
 * A logger function called to log selected plug-in framework messages. The
 * messages may be localized. Plug-in framework core functions must not
 * be called from within a logger function invocation. In a multi-threaded
 * environment logger function invocations are serialized by the framework.
 *
 * @param severity the severity of the message (one of CP_LOG_... constants)
 * @param msg the message to be logged, possibly localized
 * @param apid the identifier of the activating plug-in or NULL for the client program
 * @param user_data the user data pointer given when the logger was registered
 * @see cp_add_logger
 */
typedef void (*cp_logger_func_t)(cp_log_severity_t severity, const char *msg, const char *apid, void *user_data);

/*@}*/


/** @name Error handling */
/*@{*/

/**
 * A fatal error handler for handling unrecoverable errors. If the error
 * handler returns then the framework aborts the program. Plug-in framework
 * functions must not be called from within a fatal error handler invocation.
 *
 * @param msg the possibly localized error message
 * @see cp_set_fatal_error_handler
 */
typedef void (*cp_fatal_error_func_t)(const char *msg);

/*@}*/
/*@}*/


/* ------------------------------------------------------------------------
 * Function declarations
 * ----------------------------------------------------------------------*/

/**
 * @name Functions
 *
 * The C-Pluff framework core functions are defined here. The functions and
 * any data exposed by them are thread-safe if the library has been compiled
 * with multi-threading support. The initialization functions \Ref{cp_init},
 * \Ref{cp_destroy} and \Ref{cp_set_fatal_error_handler} are exceptions,
 * they are not thread-safe.
 */
/*@{*/

/**
 * @name Library information
 *
 * This function can be used to query runtime information about the
 * library implementation. It may be used by the client program or
 * by a plug-in runtime.
 */
/*@{*/

/**
 * Returns static information about the library implementation.
 * This function can be called at any time.
 *
 * @return static information about the library implementation
 */
CP_API cp_core_info_t *cp_get_core_info(void);

/*@}*/


/**
 * @name Library initialization
 *
 * These functions are used for library and framework initialization.
 * They are intended to be used by the client program.
 */
/*@{*/

/**
 * Initializes the plug-in framework. This function must be called
 * by the client program before calling any other plug-in framework
 * functions except \Ref{cp_get_implementation_info} and
 * \Ref{cp_set_fatal_error_handler}. This function may be
 * called several times but it is not thread-safe. Library resources
 * should be released by calling \Ref{cp_destroy} when the framework is
 * not needed anymore.
 *
 * Additionally, to enable localization support, the client program should
 * set the current locale using setlocale(LC_ALL, "") before calling
 * this function.
 *
 * @return CP_OK (zero) on success or error code on failure
 */
CP_API int cp_init(void);

/**
 * Destroys the plug-in framework and releases the resources used by it.
 * The plug-in framework is only destroyed after this function has
 * been called as many times as \Ref{cp_init}. This function is not
 * thread-safe. Plug-in framework functions other than \Ref{cp_init},
 * \Ref{cp_get_implementation_info} and \Ref{cp_set_fatal_error_handler}
 * must not be called after the plug-in framework has been destroyed.
 * All contexts are destroyed and all data references returned by the
 * framework become invalid.
 */
CP_API void cp_destroy(void);

/**
 * Sets the fatal error handler called on non-recoverable errors. The default
 * error handler prints the error message out to standard error and aborts
 * the program. If the user specified error handler returns then the framework
 * will abort the program. Setting NULL error handler will restore the default
 * handler. This function is not thread-safe and it should be called
 * before initializing the framework to catch all fatal errors.
 * 
 * @param error_handler the fatal error handler
 */
CP_API void cp_set_fatal_error_handler(cp_fatal_error_func_t error_handler);

/*@}*/


/**
 * @name Logging
 *
 * These functions can be used to log plug-in framework messages.
 * They can be used by the client program or by a plug-in runtime.
 */
/*@{*/

/**
 * Registers a new logger or updates the settings of a registered logger.
 * If the specified logger is not yet known, a new logger registration
 * is made, otherwise the settings for the existing logger are updated.
 * If a plug-in registers a logger it must remove the logger registration
 * when the plug-in is stopped.
 *
 * @param logger the logger function to be called
 * @param user_data the user data pointer passed to the logger
 * @param min_severity the minimum severity of messages passed to logger
 * @param ctx_rule only the messages associated with the specified context
 *			are passed to logger if non-NULL
 * @return CP_OK (zero) on success or CP_ERR_RESOURCE if insufficient memory
 */
CP_API int cp_add_logger(cp_logger_func_t logger, void *user_data, cp_log_severity_t min_severity, cp_context_t *ctx_rule);

/**
 * Removes a logger registration.
 *
 * @param logger the logger function to be unregistered
 */
CP_API void cp_remove_logger(cp_logger_func_t logger);

/*@}*/


/**
 * @name Plug-in context management
 *
 * These functions are used to manage plug-in contexts from the client
 * program perspective. They are not intended to be used by a plug-in runtime.
 * From the client program perspective a plug-in context is a container for
 * installed plug-ins. There can be several plug-in context instances if there
 * are several independent sets of plug-ins. However, different plug-in
 * contexts are not very isolated from each other in practice because the
 * symbols exported by a plug-in runtime in one context are visible to
 * all plug-ins in all context instances. This also means that having the
 * same plug-in concurrently loaded in two different contexts causes problems.
 */
/*@{*/

/**
 * Creates a new plug-in context which can be used as a container for plug-ins.
 * Plug-ins are loaded and installed into a specific context. The client
 * program may have more than one plug-in context but the plug-ins that
 * interact with each other should be placed in the same context. The
 * resources associated with the context are released by calling
 * \Ref{cp_destroy_context} when the context is not needed anymore.
 * 
 * @param error pointer to the location where error code or CP_OK is stored, or NULL
 * @return the newly created plugin context, or NULL on failure
 */
CP_API cp_context_t * cp_create_context(int *error);

/**
 * Destroys the specified plug-in context and releases the associated resources.
 * Stops and uninstalls all plug-ins in the context. The context must not be
 * accessed after calling this function.
 * 
 * @param ctx the context to be destroyed
 */
CP_API void cp_destroy_context(cp_context_t *ctx);

/**
 * Registers a directory of plug-ins with a plug-in context. The
 * plug-in context will scan the directory when \Ref{cp_load_plugins} is called.
 * Returns CP_OK if the directory has already been registered.
 * 
 * @param ctx the plug-in context
 * @param dir the directory
 * @return CP_OK (zero) on success, or CP_ERR_RESOURCE if insufficient system resources
 */
CP_API int cp_add_plugin_dir(cp_context_t *ctx, const char *dir);

/**
 * Unregisters a previously registered directory of plug-ins from a plug-in context.
 * Does not delete the directory itself. Plug-ins already loaded from the
 * removed directory are not affected. Does nothing
 * if the directory has not been registered.
 * 
 * @param ctx the plug-in context
 * @param dir the previously registered directory
 */
CP_API void cp_remove_plugin_dir(cp_context_t *ctx, const char *dir);

/*@}*/


/**
 * @name Plug-in management
 *
 * These functions can be used to manage plug-ins. They are intended to be
 * used by the client program.
 */
/*@{*/

/**
 * Loads a plug-in descriptor from the specified plug-in installation
 * path and returns information about the plug-in. The plug-in descriptor
 * is validated during loading. Possible errors are reported via the
 * specified plug-in context (if non-NULL). The plug-in is
 * not installed to any context. If operation fails or the descriptor
 * is invalid then NULL is returned. The caller must release the returned
 * information by calling \Ref{cp_release_plugin_info} when it does not
 * need the information anymore, typically after installing the plug-in.
 * 
 * @param ctx the plug-in context for error reporting, or NULL for none
 * @param path the installation path of the plug-in
 * @param error pointer to the location for the returned error code, or NULL
 * @return pointer to the information structure or NULL if error occurs
 */
CP_API cp_plugin_info_t * cp_load_plugin_descriptor(cp_context_t *ctx, const char *path, int *error);

/**
 * Installs the plug-in described by the specified plug-in information
 * structure to the specified plug-in context. The plug-in information
 * must have been obtained from \Ref{cp_load_plugin_descriptor}.
 * The installation fails on \Ref{CP_ERR_CONFLICT} if the context already
 * has an installed plug-in with the same plug-in identifier. Installation
 * also fails if the plug-in tries to install an extension point which
 * conflicts with an already installed extension point.
 * The plug-in information must not be modified but it is safe to call
 * \Ref{cp_release_plugin_info} after the plug-in has been installed.
 *
 * @param ctx the plug-in context
 * @param pi plug-in information structure
 * @return CP_OK (zero) on success or error code on failure
 */
CP_API int cp_install_plugin(cp_context_t *ctx, cp_plugin_info_t *pi);

/**
 * Scans for plug-ins in the registered plug-in directories, installing
 * new plug-ins and upgrading installed plug-ins. This function can be used to
 * initially load the plug-ins and to later rescan for new plug-ins.
 * 
 * When several versions of the same plug-in is available the most recent
 * version will be installed. The upgrade behavior depends on the specified
 * flags. If \Ref{CP_LP_UPGRADE} is set then upgrades to installed plug-ins are
 * allowed. The old version is unloaded and the new version installed instead.
 * If \Ref{CP_LP_STOP_ALL_ON_UPGRADE} is set then all active plug-ins are stopped
 * if any plug-ins are to be upgraded. If \Ref{CP_LP_STOP_ALL_ON_INSTALL} is set then
 * all active plug-ins are stopped if any plug-ins are to be installed or
 * upgraded. Finally, if \Ref{CP_LP_RESTART_ACTIVE} is set all currently active
 * plug-ins will be restarted after the changes (if they were stopped).
 * 
 * When removing plug-in files from the plug-in directories, the
 * plug-ins to be removed must be first unloaded. Therefore this function
 * does not check for removed plug-ins.
 * 
 * @param ctx the plug-in context
 * @param flags the bitmask of flags
 * @return CP_OK (zero) on success, an error code on failure
 */
CP_API int cp_scan_plugins(cp_context_t *ctx, int flags);

/**
 * Starts a plug-in. Also starts any imported plug-ins. If the plug-in is
 * already starting then
 * this function blocks until the plug-in has started or failed to start.
 * If the plug-in is already active then this function returns immediately.
 * If the plug-in is stopping then this function blocks until the plug-in
 * has stopped and then starts the plug-in.
 * 
 * @param ctx the plug-in context
 * @param id identifier of the plug-in to be started
 * @return CP_OK (zero) on success, an error code on failure
 */
CP_API int cp_start_plugin(cp_context_t *ctx, const char *id);

/**
 * Stops a plug-in. First stops any dependent plug-ins that are currently
 * active. Then stops the specified plug-in. If the plug-in is already
 * stopping then this function blocks until the plug-in has stopped. If the
 * plug-in is already stopped then this function returns immediately. If the
 * plug-in is starting then this function blocks until the plug-in has
 * started (or failed to start) and then stops the plug-in.
 * 
 * @param ctx the plug-in context
 * @param id identifier of the plug-in to be stopped
 * @return CP_OK (zero) on success or CP_ERR_UNKNOWN if unknown plug-in
 */
CP_API int cp_stop_plugin(cp_context_t *ctx, const char *id);

/**
 * Stops all active plug-ins.
 * 
 * @param ctx the plug-in context
 */
CP_API void cp_stop_all_plugins(cp_context_t *ctx);

/**
 * Uninstalls the specified plug-in. The plug-in is first stopped if it is active.
 * Then uninstalls the plug-in and any dependent plug-ins. The client program
 * must not have any unreleased references to dynamic symbols provided by the
 * uninstalled plug-ins.
 * 
 * @param ctx the plug-in context
 * @param id identifier of the plug-in to be unloaded
 * @return CP_OK (zero) on success or CP_ERR_UNKNOWN if unknown plug-in
 */
CP_API int cp_uninstall_plugin(cp_context_t *ctx, const char *id);

/**
 * Uninstalls all plug-ins. All plug-ins are first stopped and then
 * uninstalled. The client program must not have any
 * unreleased references to dynamic symbols provided by the uninstalled
 * plug-ins.
 * 
 * @param ctx the plug-in context
 */
CP_API void cp_uninstall_all_plugins(cp_context_t *ctx);

/*@}*/


/**
 * @name Context data
 *
 * These functions can be used to access context specific plug-in or client
 * program data. They may be used by the client program or by a plug-in
 * runtime.
 */
/*@{*/

/**
 * Sets the context data pointer.
 *
 * @param ctx the plug-in context
 * @param user_data the pointer to user data, or NULL to unset
 */
CP_API void cp_set_context_data(cp_context_t *ctx, void *user_data);

/**
 * Returns the context data pointer.
 *
 * @param ctx the plug-in context
 * @return a pointer to user data, or NULL if not set
 */
CP_API void * cp_get_context_data(cp_context_t *ctx);

/*@}*/


/**
 * @name Plug-in and extension information
 *
 * These functions can be used to query information about the installed
 * plug-ins, extension points and extensions or to listen for plug-in state
 * changes. They may be used by the client program or by a plug-in runtime.
 */
/*@{*/

/**
 * Returns static information about the specified plug-in. The returned
 * data structures must not be modified and the caller must
 * release the information by calling \Ref{cp_release_info} when the
 * information is not needed anymore.
 * 
 * @param ctx the plug-in context
 * @param id identifier of the plug-in to be examined
 * @param error filled with an error code, if non-NULL
 * @return pointer to the information structure or NULL on failure
 */
CP_API cp_plugin_info_t * cp_get_plugin_info(cp_context_t *ctx, const char *id, int *error);

/**
 * Returns static information about the installed plug-ins. The returned
 * data structures must not be modified and the caller must
 * release the information by calling \Ref{cp_release_info} when the
 * information is not needed anymore.
 * 
 * @param ctx the plug-in context
 * @param error filled with an error code, if non-NULL
 * @param num filled with the number of returned plug-ins, if non-NULL
 * @return pointer to a NULL-terminated list of pointers to plug-in information
 * 			or NULL on failure
 */
CP_API cp_plugin_info_t ** cp_get_plugins_info(cp_context_t *ctx, int *error, int *num);

/**
 * Returns static information about the currently installed extension points.
 * The returned data structures must not be modified and the caller must
 * release the information by calling \Ref{cp_release_info} when the
 * information is not needed anymore.
 *
 * @param ctx the plug-in context
 * @param error filled with an error code, if non-NULL
 * @param num filled with the number of returned extension points, if non-NULL
 * @return pointer to a NULL-terminated list of pointers to extension point
 *			information or NULL on failure
 */
CP_API cp_ext_point_t ** cp_get_ext_points_info(cp_context_t *ctx, int *error, int *num);

/**
 * Returns static information about the currently installed extension points.
 * The returned data structures must not be modified and the caller must
 * release the information by calling \Ref{cp_release_info} when the
 * information is not needed anymore.
 *
 * @param ctx the plug-in context
 * @param extpt_id the extension point identifier or NULL for all extensions
 * @param error filled with an error code, if non-NULL
 * @param num filled with the number of returned extension points, if non-NULL
 * @return pointer to a NULL-terminated list of pointers to extension
 *			information or NULL on failure
 */
CP_API cp_extension_t ** cp_get_extensions_info(cp_context_t *ctx, const char *extpt_id, int *error, int *num);

/**
 * Releases previously obtained dynamically allocated information. The
 * documentation for functions returning such information refers
 * to this function. The information must not be accessed after it has
 * been released. The framework uses reference counting to deallocate
 * the information when it is not in use anymore.
 * 
 * @param info the information to be released
 */
CP_API void cp_release_info(void *info);

/**
 * Returns the current state of the specified plug-in. Returns
 * CP_PLUGIN_UNINSTALLED if the specified plug-in identifier is unknown.
 * 
 * @param ctx the plug-in context
 * @param id the plug-in identifier
 * @return the current state of the plug-in
 */
CP_API cp_plugin_state_t cp_get_plugin_state(cp_context_t *ctx, const char *id);

/**
 * Registers a plug-in listener with a plug-in context. The listener is called
 * synchronously immediately after a plug-in state change. There can be several
 * listeners registered with the same context. If a plug-in registers a
 * listener it must unregister it when the plug-in is stopped.
 * 
 * @param ctx the plug-in context
 * @param listener the plug-in listener to be added
 * @param user_data user data pointer supplied to the listener
 * @return CP_OK (zero) on success, CP_ERR_RESOURCE if out of resources
 */
CP_API int cp_add_plugin_listener(cp_context_t *ctx, cp_plugin_listener_func_t listener, void *user_data);

/**
 * Removes a plug-in listener from a plug-in context. Does nothing if the
 * specified listener was not registered.
 * 
 * @param ctx the plug-in context
 * @param listener the plug-in listener to be removed
 */
CP_API void cp_remove_plugin_listener(cp_context_t *ctx, cp_plugin_listener_func_t listener);

/**
 * Traverses a configuration element tree and returns the specified element.
 * The target element is specified by a base element and a relative path from
 * the base element to the target element. The path includes element names
 * separated by slash '/'. Two dots ".." can be used to designate a parent
 * element. Returns NULL if the specified element does not exist. If there are
 * several subelements with the same name, this function chooses the first one
 * when traversing the tree.
 *
 * @param base the base configuration element
 * @param path the path to the target element
 * @return the target element or NULL if nonexisting
 */
CP_API cp_cfg_element_t * cp_lookup_cfg_element(cp_cfg_element_t *base, const char *path);

/**
 * Traverses a configuration element tree and returns the value of the
 * specified element or attribute. The target element or attribute is specified
 * by a base element and a relative path from the base element to the target
 * element or attributes. The path includes element names
 * separated by slash '/'. Two dots ".." can be used to designate a parent
 * element. The path may end with '@' followed by an attribute name
 * to select an attribute. Returns NULL if the specified element or attribute
 * does not exist or does not have a value. If there are several subelements
 * with the same name, this function chooses the first one when traversing the
 * tree.
 *
 * @param base the base configuration element
 * @param path the path to the target element
 * @return the value of the target element or attribute or NULL
 */
CP_API char * cp_lookup_cfg_value(cp_cfg_element_t *base, const char *path);

/*@}*/


/**
 * @name Dynamic symbols
 *
 * These functions can be used to dynamically access symbols exported by the
 * plug-ins. They are intended to be used by a plug-in runtime.
 * The framework automatically maintains a dynamic dependency from the symbol
 * using plug-in to the symbol defining plug-in. If the symbol defining plug-in
 * is about to be stopped then the symbol using plug-in is stopped as well.
 */
/*@{*/

/**
 * Defines a context specific symbol. If a plug-in has symbols which have
 * a plug-in instance specific value then the plug-in should define those
 * symbols when it is started. The defined symbols are cleared
 * automatically when the plug-in instance is stopped. Symbols can not be
 * redefined.
 * 
 * @param ctx the plug-in context
 * @param name the name of the symbol
 * @param ptr pointer value for the symbol
 * @return CP_OK (zero) on success or an error code on failure
 */
CP_API int cp_define_symbol(cp_context_t *ctx, const char *name, void *ptr);

/**
 * Resolves a named symbol provided by the specified plug-in which is started
 * automatically if it is not already active. The symbol may
 * be context specific. The framework first looks for a context specific
 * symbol and then falls back to resolving a global symbol exported by the
 * plug-in runtime library. The plug-in framework creates dynamically a
 * dependency from the symbol using
 * plug-in to the symbol defining plug-in. The symbol must be released using
 * \Ref{cp_release_symbol} when it is not needed anymore or at latest when
 * the symbol using plug-in is stopped. Pointers to dynamically resolved
 * symbols must not be passed on to other plug-ins.
 *
 * @param ctx the plug-in context
 * @param id the identifier of the symbol defining plug-in
 * @param name the name of the symbol
 * @param error filled with an error code if non-NULL
 * @return the pointer associated with the symbol or NULL on failure
 */
CP_API void *cp_resolve_symbol(cp_context_t *ctx, const char *id, const char *name, int *error);

/**
 * Releases a previously obtained symbol. The pointer must not be used by the
 * releasing plug-in after the symbol has been released. The symbol is released
 * only after as many calls to this function as there have been for
 * \Ref{cp_resolve_symbol} for the same plug-in and symbol.
 *
 * @param ctx the plug-in context
 * @param ptr the pointer associated with the symbol
 */
CP_API void cp_release_symbol(cp_context_t *ctx, void *ptr);

/*@}*/


/*@}*/
/*@}*/

#ifdef __cplusplus
}
#endif /*__cplusplus*/

#endif /*CPLUFF_H_*/
