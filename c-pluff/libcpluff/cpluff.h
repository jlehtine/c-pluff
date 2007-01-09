/*-------------------------------------------------------------------------
 * C-Pluff, a plug-in framework for C
 * Copyright 2006 Johannes Lehtinen
 *-----------------------------------------------------------------------*/

/** @file 
 * C-Pluff core C API header file.
 * The elements declared here constitute the C-Pluff core C API. To use the
 * API include this file and link the main program and plug-in runtime
 * libraries with the C-Pluff core library which has a base name cpluff. The
 * actual name of the library file is platform dependent, for example
 * libcpluff.so or cpluff.dll. In addition to local declarations, this file
 * also includes cpluffdef.h header file for defines common to C and C++ API.
 */

#ifndef CPLUFF_H_
#define CPLUFF_H_

/**
 * @defgroup coreC C-Pluff Core C API
 * 
 * The elements declared here constitute the C-Pluff core C API.
 */

/**
 * @defgroup defines Defines
 * @ingroup coreC
 * Preprocessor defines.
 */
 
#include "cpluffdef.h"

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/


/* ------------------------------------------------------------------------
 * Constants
 * ----------------------------------------------------------------------*/

/**
 * @defgroup errorCodes Error codes
 * @ingroup defines
 *
 * Error codes returned by API functions.
 * Most of the interface functions return error codes. The returned
 * error code either indicates successful completion of the operation
 * or some specific kind of error. Some functions do not return an error
 * code because they never fail.
 */
/*@{*/

/**
 * Operation was performed successfully (equals to zero).
 * @showinitializer
 */
#define CP_OK 0

/** Not enough memory or other operating system resources available */
#define CP_ERR_RESOURCE 2

/** The specified object is unknown to the framework */
#define CP_ERR_UNKNOWN 3

/** An I/O error occurred */
#define CP_ERR_IO 4

/** Malformed plug-in descriptor was encountered when loading a plug-in */
#define CP_ERR_MALFORMED 5

/** Plug-in or symbol conflicts with another plug-in or symbol. */
#define CP_ERR_CONFLICT 6

/** Plug-in dependencies could not be satisfied. */
#define CP_ERR_DEPENDENCY 7

/** Plug-in runtime signaled an error. */
#define CP_ERR_RUNTIME 8

/*@}*/


/**
 * @defgroup scanFlags Flags for plug-in scanning
 * @ingroup defines
 *
 * These constants can be orred together for the flags
 * parameter of ::cp_scan_plugins.
 */
/*@{*/

/** 
 * This flag enables upgrades of installed plug-ins by unloading
 * the old version and installing the new version.
 */
#define CP_LP_UPGRADE 0x01

/**
 * This flag causes all plug-ins to be stopped before any
 * plug-ins are to be upgraded.
 */
#define CP_LP_STOP_ALL_ON_UPGRADE 0x02

/**
 * This flag causes all plug-ins to be stopped before any
 * plugins are to be installed (also if new version is to be installed
 * as part of an upgrade).
 */
#define CP_LP_STOP_ALL_ON_INSTALL 0x04

/**
 * Setting this flag causes the currently active plug-ins to be restarted
 * after all changes to the plug-ins have been made (if they were stopped).
 */
#define CP_LP_RESTART_ACTIVE 0x08

/*@}*/


/* ------------------------------------------------------------------------
 * Data types
 * ----------------------------------------------------------------------*/

/**
 * @defgroup enums Enumerations
 * @ingroup coreC
 * Constant value enumerations.
 */

/**
 * @defgroup typedefs Typedefs
 * @ingroup coreC
 * Typedefs of various kind.
 */

/**
 * @defgroup structs Data structures
 * @ingroup coreC
 * Data structure definitions.
 */
 

/* Enumerations */

/**
 * @ingroup enums
 * An enumeration of possible plug-in states. Plug-in states are controlled
 * by @ref funcsPlugin "plug-in management functions". Plug-in states can be
 * observed by @ref cp_add_plugin_listener "registering" a
 * @ref cp_plugin_listener_func_t "plug-in listener function"
 * or by calling ::cp_get_plugin_state.
 *
 * @sa cp_plugin_listener_t
 * @sa cp_get_plugin_state
 */
enum cp_plugin_state_t {

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
	 * A plug-in is started when explicitly requested by the main
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
	 * requested by the main program or when its dependencies are being
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
	
};

/**
 * @ingroup enums
 * An enumeration of possible message severities for framework logging. These
 * constants are used when passing a log message to a
 * @ref cp_logger_func_t "logger function" and when
 * @ref cp_add_logger "registering" a logger function.
 */
enum cp_log_severity_t {

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
	
};

/*@}*/


/* Typedefs */

/**
 * @defgroup typedefsOpaque Opaque types
 * @ingroup typedefs
 * Opaque data type definitions.
 */
/*@{*/
 
/**
 * A plug-in context represents the co-operation environment of a set of
 * plug-ins from the perspective of a particular participating plug-in or
 * the perspective of the main program. It is used as an opaque handle to
 * the shared resources but the framework also uses the context to identify
 * the plug-in or the main program invoking framework functions. Therefore
 * a plug-in should not generally expose its context instance to other
 * plug-ins or the main program and neither should the main program
 * expose its context instance to plug-ins. The main program creates
 * plug-in contexts using ::cp_create_context and plug-ins receive their
 * plug-in contexts via @ref cp_plugin_runtime_t::create.
 */
typedef struct cp_context_t cp_context_t;

/*@}*/

 /**
  * @defgroup typedefsShorthand Shorthand type names
  * @ingroup typedefs
  * Shorthand type names for structs and enumerations.
  */
/*@{*/

/** A type for cp_core_info_t structure. */
typedef struct cp_core_info_t cp_core_info_t;

/** A type for cp_plugin_info_t structure. */
typedef struct cp_plugin_info_t cp_plugin_info_t;

/** A type for cp_plugin_import_t structure. */
typedef struct cp_plugin_import_t cp_plugin_import_t;

/** A type for cp_ext_point_t structure. */
typedef struct cp_ext_point_t cp_ext_point_t;

/** A type for cp_extension_t structure. */
typedef struct cp_extension_t cp_extension_t;

/** A type for cp_cfg_element_t structure. */
typedef struct cp_cfg_element_t cp_cfg_element_t;

/** A type for cp_plugin_runtime_t structure. */
typedef struct cp_plugin_runtime_t cp_plugin_runtime_t;

/** A type for cp_plugin_state_t enumeration. */
typedef enum cp_plugin_state_t cp_plugin_state_t;

/** A type for cp_log_severity_t enumeration. */
typedef enum cp_log_severity_t cp_log_severity_t;

/*@}*/

/**
 * @defgroup typedefsFuncs Callback function types
 * @ingroup typedefs
 * Typedefs for client supplied callback functions.
 */
/*@{*/

/**
 * A listener function called synchronously after a plugin state change.
 * The function should return promptly.
 * @ref funcsInit "Library initialization",
 * @ref funcsContext "plug-in context management",
 * @ref funcsPlugin "plug-in management",
 * listener registration (::cp_add_plugin_listener and ::cp_remove_plugin_listener)
 * and @ref funcsSymbols "dynamic symbol" functions must not be called from
 * within a plug-in listener invocation. Listener functions are registered
 * using ::cp_add_plugin_listener.
 * 
 * @param plugin_id the plug-in identifier
 * @param old_state the old plug-in state
 * @param new_state the new plug-in state
 * @param user_data the user data pointer supplied at listener registration
 */
typedef void (*cp_plugin_listener_func_t)(const char *plugin_id, cp_plugin_state_t old_state, cp_plugin_state_t new_state, void *user_data);

/**
 * A logger function called to log selected plug-in framework messages. The
 * messages may be localized. Plug-in framework core functions must not
 * be called from within a logger function invocation. In a multi-threaded
 * environment logger function invocations are serialized by the framework.
 * Logger functions are registered using ::cp_add_logger.
 *
 * @param severity the severity of the message
 * @param msg the message to be logged, possibly localized
 * @param apid the identifier of the activating plug-in or NULL for the main program
 * @param user_data the user data pointer given when the logger was registered
 */
typedef void (*cp_logger_func_t)(cp_log_severity_t severity, const char *msg, const char *apid, void *user_data);

/**
 * A fatal error handler for handling unrecoverable errors. If the error
 * handler returns then the framework aborts the program. Plug-in framework
 * core functions must not be called from within a fatal error handler
 * invocation. The fatal error handler function is set using
 * ::cp_set_fatal_error_handler.
 *
 * @param msg the possibly localized error message
 * @sa cp_set_fatal_error_handler
 */
typedef void (*cp_fatal_error_func_t)(const char *msg);

/*@}*/


/* Data structures */

/**
 * @ingroup structs
 * Core information structure contains information about
 * the C-Pluff core implementation. This information can be
 * obtained at runtime by calling ::cp_get_core_info. For compile time
 * version information, see @ref versionInfo "version information defines".
 */
struct cp_core_info_t {

	/** @copydoc CP_RELEASE_VERSION */
	const char *release_version;
	
	/** @copydoc CP_CORE_API_VERSION */
	int core_api_version;
	
	/** @copydoc CP_CORE_API_REVISION */
	int core_api_revision;
	
	/** @copydoc CP_CORE_API_AGE */
	int core_api_age;
  
	/**
	 * The canonical host type. This is the canonical host
	 * type for which the framework core was built for.
	 */
	const char *host_type;
	
	/** The type of multi-threading support, or NULL for none. */
	const char *multi_threading_type;
	
};

/**
 * @ingroup structs
 * Plug-in information structure captures information about a plug-in. This
 * information can be loaded from a plug-in descriptor using
 * ::cp_load_plugin_descriptor. Information about installed plug-ins can
 * be obtained using ::cp_get_plugin_info and ::cp_get_plugins_info. This
 * structure corresponds to the @a plugin element in a plug-in descriptor.
 */
struct cp_plugin_info_t {
	
	/**
	 * The obligatory unique identifier of the plugin. A recommended way
	 * to generate identifiers is to use domain name service (DNS) prefixes
	 * (for example, org.cpluff.ExamplePlugin) to avoid naming conflicts. This
	 * corresponds to the @a id attribute of the @a plugin element in a plug-in
	 * descriptor.
	 */
	char *identifier;
	
	/**
	 * An optional plug-in name. NULL if not available. The plug-in name is
	 * intended only for display purposes and the value can be localized.
	 * This corresponds to the @a name attribute of the @a plugin element in
	 * a plug-in descriptor.
	 */
	char *name;
	
	/**
	 * An optional release version string. NULL if not available. The
	 * release version is intended only for display purposes (compatibility
	 * checks use API version information in @ref cp_plugin_api_t). This
	 * corresponds to the @a version attribute of the @a plugin element in
	 * a plug-in descriptor.
	 */
	char *version;
	
	/**
	 * An optional provider name. NULL if not available. This is the name of
	 * the author or the organization providing the plug-in. The
	 * provider name is intended only for display purposes and the value can
	 * be localized. This corresponds to the @a provider-name attribute of the
	 * @a plugin element in a plug-in descriptor.
	 */
	char *provider_name;
	
	/**
	 * Optional API version information. -1 if not available. If the plug-in
	 * provides any API interfaces (such as extension points or global symbols)
	 * then it should also declare versioning for the API. This corresponds to
	 * the @a version attribute of the @a api element in a plug-in descriptor.
	 */
	int api_version;
	
	/**
	 * Optional API revision information. -1 if API is not versioned.
	 * This corresponds to the @a revision attribute of the @a api element in
	 * a plug-in descriptor.
	 */
	int api_revision;
	
	/**
	 * Optional API age information. -1 if API is not versioned.
	 * Subtracting the API age from the current API version gives the earliest
	 * API version supported (backwards compatibility) by the current API.
	 * This corresponds to the @a age attribute of the @a api element in a
	 * plug-in descriptor.
	 */
	int api_age;
	
	/**
	 * Path of the plugin directory, or NULL if not known. This is the
	 * (absolute or relative) path to the plug-in directory containing
	 * plug-in data and the plug-in runtime library. The value corresponds
	 * to the path specified to ::cp_load_plugin_descriptor when loading
	 * the plug-in.
	 */
	char *plugin_path;
	
	/** Number of import entries in the @ref imports array. */
	unsigned int num_imports;
	
	/**
	 * An array of @ref num_imports import entries. These correspond to
	 * @a import elements in a plug-in descriptor.
	 */
	cp_plugin_import_t *imports;

    /**
     * The plug-in runtime library path, relative to the plug-in directory,
     * or NULL if none. This corresponds to the @a library attribute of the
     * @a runtime element in a plug-in descriptor. 
     */
    char *lib_path;
    
    /**
     * The symbol pointing to the plug-in runtime function information or
     * NULL if none. The symbol with this name should point to an instance of
     * @ref cp_plugin_runtime_t structure. This corresponds to the
     * @a funcs attribute of the @a runtime element in a plug-in descriptor. 
     */
    char *runtime_funcs_symbol;
    
	/** Number of extension points in @ref ext_points array. */
	unsigned int num_ext_points;
	
	/**
	 * An array of @ref num_ext_points extension points provided by this
	 * plug-in. These correspond to @a extension-point elements in a
	 * plug-in descriptor.
	 */
	cp_ext_point_t *ext_points;
	
	/** Number of extensions in @ref extensions array. */
	unsigned int num_extensions;
	
	/**
	 * An array of @ref num_extensions extensions provided by this
	 * plug-in. These correspond to @a extension elements in a plug-in
	 * descriptor.
	 */
	cp_extension_t *extensions;

};

/**
 * @ingroup structs
 * Information about plug-in import. Plug-in import structures are
 * contained in @ref cp_plugin_info_t::imports.
 */
struct cp_plugin_import_t {
	
	/**
	 * The identifier of the imported plug-in. This corresponds to the
	 * @a plugin attribute of the @a import element in a plug-in descriptor.
	 */
	char *plugin_id;
	
	/**
	 * An optional API version requirement. -1 if no version requirement.
	 * This corresponds to the @a api-version attribute of the @a import
	 * element in a plug-in descriptor.
	 */
	int api_version;
	
	/**
	 * Is this import optional. 1 for optional and 0 for mandatory import.
	 * An optional import causes the imported plug-in to be started if it is
	 * available but does not stop the importing plug-in from starting if the
	 * imported plug-in is not available. If the imported plug-in is available
	 * but the API version conflicts with the API version requirement then the
	 * importing plug-in fails to start. This corresponds to the @a optional
	 * attribute of the @a import element in a plug-in descriptor.
	 */
	int optional;
};

/**
 * @ingroup structs
 * Extension point structure captures information about an extension
 * point. Extension point structures are contained in
 * @ref cp_plugin_info_t::ext_points.
 */
struct cp_ext_point_t {

	/**
	 * A pointer to plug-in information containing this extension point.
	 * This reverse pointer is provided to make it easy to get information
	 * about the plug-in which is hosting a particular extension point.
	 */
	cp_plugin_info_t *plugin;
	
	/**
	 * The local identifier uniquely identifying the extension point within the
	 * host plug-in. This corresponds to the @name id attribute of an
	 * @a extension-point element in a plug-in descriptor.
	 */
	char *local_id;
	
	/**
	 * The unique identifier of the extension point. This is automatically
	 * constructed by concatenating the identifier of the host plug-in and
	 * the local identifier of the extension point.
	 */
	char *global_id;

	/**
	 * An optional extension point name. NULL if not available. The extension
	 * point name is intended for display purposes only and the value can be
	 * localized. This corresponds to the @a name attribute of
	 * an @a extension-point element in a plug-in descriptor.
	 */
	char *name;
	
	/**
	 * An optional path to the extension schema definition.
	 * NULL if not available. The path is relative to the plug-in directory.
	 * This corresponds to the @a schema attribute
	 * of an @a extension-point element in a plug-in descriptor.
	 */
	char *schema_path;
};

/**
 * @ingroup structs
 * Extension structure captures information about an extension. Extension
 * structures are contained in @ref cp_plugin_info_t::extensions.
 */
struct cp_extension_t {

	/** 
	 * A pointer to plug-in information containing this extension.
	 * This reverse pointer is provided to make it easy to get information
	 * about the plug-in which is hosting a particular extension.
	 */
	cp_plugin_info_t *plugin;
	
	/**
	 * The unique identifier of the extension point this extension is
	 * attached to. This corresponds to the @a point attribute of an
	 * @a extension element in a plug-in descriptor.
	 */
	char *ext_point_id;
	
	/**
	 * An optional local identifier uniquely identifying the extension within
	 * the host plug-in. NULL if not available. This corresponds to the
	 * @a id attribute of an @a extension element in a plug-in descriptor.
	 */
	char *local_id;

    /**
     * An optional unique identifier of the extension. NULL if not available.
     * This is automatically constructed by concatenating the identifier
     * of the host plug-in and the local identifier of the extension.
     */
    char *global_id;
	 
	/** 
	 * An optional extension name. NULL if not available. The extension name
	 * is intended for display purposes only and the value can be localized.
	 * This corresponds to the @a name attribute
	 * of an @a extension element in a plug-in descriptor.
	 **/
	char *name;
	
	/**
	 * Extension configuration starting with the extension element.
	 * This includes extension configuration information as a tree of
	 * configuration elements. These correspond to the @a extension
	 * element and its contents in a plug-in descriptor.
	 */
	cp_cfg_element_t *configuration;
};

/**
 * @ingroup structs
 * A configuration element contains configuration information for an
 * extension. Utility functions ::cp_lookup_cfg_element and
 * ::cp_lookup_cfg_value can be used for traversing the tree of
 * configuration elements. Pointer to the root configuration element is
 * stored at @ref cp_extension_t::configuration and others are contained as
 * @ref cp_cfg_element_t::children "children" of parent elements.
 */
struct cp_cfg_element_t {
	
	/**
	 * The name of the configuration element. This corresponds to the name of
	 * the element in a plug-in descriptor.
	 */
	char *name;

	/** Number of attribute name, value pairs in the @ref atts array. */
	unsigned int num_atts;
	
	/**
	 * An array of pointers to alternating attribute names and values.
	 * Attribute values can be localized.
	 */
	char **atts;
	
	/**
	  * An optional value of this configuration element. NULL if not available.
	  * The value can be localized. This corresponds to the
	  * text contents of the element in a plug-in descriptor.
	  */
	char *value;
	
	/** A pointer to the parent element or NULL if this is a root element. */
 	cp_cfg_element_t *parent;
 	
 	/** The index of this element among its siblings (0-based). */
 	unsigned int index;
 	
 	/** Number of children in the @ref children array. */
 	unsigned int num_children;

	/**
	 * An array of @ref num_children childrens of this element. These
	 * correspond to child elements in a plug-in descriptor.
	 */
	cp_cfg_element_t *children;
};

/**
 * @ingroup structs
 * A plug-in runtime structure containing pointers to plug-in
 * control functions. A plug-in runtime defines a static instance of this
 * structure to pass information about the available control functions
 * to the plug-in framework. The plug-in framework then uses the
 * functions to create and control plug-in instances. The symbol pointing
 * to the runtime information instance is named by the @a funcs
 * attribute of the @a runtime element in a plug-in descriptor.
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
	 * must return NULL. Core framework functions must not be called
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
	 * called to clean up plug-in state. @ref funcsInit "Library initialization",
	 * @ref funcsContext "plug-in context management" and
	 * @ref funcsPlugin "plug-in management" functions must not be
	 * called from within a start function invocation. The start function
	 * pointer can be NULL if the plug-in runtime does not have a start
	 * function.
	 * 
	 * @param data an opaque pointer to plug-in instance data
	 * @return non-zero on success, or zero on failure
	 */
	int (*start)(void *data);
	
	/**
	 * A stop function called to stop a plugin instance.
	 * This function must cease all plug-in runtime activities.
	 * @ref funcsInit "Library initialization",
	 * @ref funcsContext "plug-in context management",
	 * @ref funcsPlugin "plug-in management"
	 * functions and ::cp_resolve_symbol must not be called from within
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
 	 * Core framework functions must not be called
	 * from within a destroy function invocation.
	 *
	 * @param data an opaque pointer to plug-in instance data
	 */
	void (*destroy)(void *data);

};

/*@}*/


/* ------------------------------------------------------------------------
 * Function declarations
 * ----------------------------------------------------------------------*/

/**
 * @defgroup funcs Functions
 * @ingroup coreC
 *
 * Global public API functions. The C-Pluff C API functions and
 * any data exposed by them are generally thread-safe if the library has been
 * compiled with multi-threading support. The initialization functions
 * ::cp_init, ::cp_destroy and ::cp_set_fatal_error_handler are
 * exceptions, they are not thread-safe.
 */

/**
 * @defgroup funcsCoreInfo Core information
 * @ingroup funcs
 *
 * This function can be used to query runtime information about the
 * library implementation. It may be used by the main program or
 * by a plug-in runtime.
 */
/*@{*/

/**
 * Returns static information about the core framework implementation.
 * The returned information must not be modified.
 * This function can be called at any time. For compile time checks,
 * see @ref versionInfo "version information defines".
 *
 * @return static information about the core framework implementation
 */
CP_API const cp_core_info_t *cp_get_core_info(void);

/*@}*/


/**
 * @defgroup funcsInit Library initialization
 * @ingroup funcs
 *
 * These functions are used for library and framework initialization.
 * They are intended to be used by the main program.
 */
/*@{*/

/**
 * Initializes the plug-in framework. This function must be called
 * by the main program before calling any other plug-in framework
 * functions except ::cp_get_core_info and
 * ::cp_set_fatal_error_handler. This function may be
 * called several times but it is not thread-safe. Library resources
 * should be released by calling ::cp_destroy when the framework is
 * not needed anymore.
 *
 * Additionally, to enable localization support, the main program should
 * set the current locale using @code setlocale(LC_ALL, "") @endcode
 * before calling this function.
 *
 * @return #CP_OK (zero) on success or error code on failure
 */
CP_API int cp_init(void);

/**
 * Destroys the plug-in framework and releases the resources used by it.
 * The plug-in framework is only destroyed after this function has
 * been called as many times as ::cp_init. This function is not
 * thread-safe. Plug-in framework functions other than ::cp_init,
 * ::cp_get_core_info and ::cp_set_fatal_error_handler
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
 * @defgroup funcsLogging Logging
 * @ingroup funcs
 *
 * These functions can be used to log plug-in framework messages.
 * They can be used by the main program or by a plug-in runtime.
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
 * @return #CP_OK (zero) on success or #CP_ERR_RESOURCE if insufficient memory
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
 * @defgroup funcsContext Plug-in context management
 * @ingroup funcs
 *
 * These functions are used to manage plug-in contexts from the main
 * program perspective. They are not intended to be used by a plug-in runtime.
 * From the main program perspective a plug-in context is a container for
 * installed plug-ins. There can be several plug-in context instances if there
 * are several independent sets of plug-ins. However, different plug-in
 * contexts are not very isolated from each other in practice because the
 * global symbols exported by a plug-in runtime in one context are visible to
 * all plug-ins in all context instances.
 */
/*@{*/

/**
 * Creates a new plug-in context which can be used as a container for plug-ins.
 * Plug-ins are loaded and installed into a specific context. The main
 * program may have more than one plug-in context but the plug-ins that
 * interact with each other should be placed in the same context. The
 * resources associated with the context are released by calling
 * ::cp_destroy_context when the context is not needed anymore.
 * 
 * @param error pointer to the location where error code or #CP_OK is stored, or NULL
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
 * plug-in context will scan the directory when ::cp_load_plugins is called.
 * Returns #CP_OK if the directory has already been registered.
 * 
 * @param ctx the plug-in context
 * @param dir the directory
 * @return #CP_OK (zero) on success, or #CP_ERR_RESOURCE if insufficient system resources
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
 * @defgroup funcsPlugin Plug-in management
 * @ingroup funcs
 *
 * These functions can be used to manage plug-ins. They are intended to be
 * used by the main program.
 */
/*@{*/

/**
 * Loads a plug-in descriptor from the specified plug-in installation
 * path and returns information about the plug-in. The plug-in descriptor
 * is validated during loading. Possible errors are reported via the
 * specified plug-in context (if non-NULL). The plug-in is
 * not installed to any context. If operation fails or the descriptor
 * is invalid then NULL is returned. The caller must release the returned
 * information by calling ::cp_release_plugin_info when it does not
 * need the information anymore, typically after installing the plug-in.
 * The returned plug-in information must not be modified.
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
 * must have been obtained from ::cp_load_plugin_descriptor.
 * The installation fails on #CP_ERR_CONFLICT if the context already
 * has an installed plug-in with the same plug-in identifier. Installation
 * also fails if the plug-in tries to install an extension point which
 * conflicts with an already installed extension point.
 * The plug-in information must not be modified but it is safe to call
 * ::cp_release_plugin_info after the plug-in has been installed.
 *
 * @param ctx the plug-in context
 * @param pi plug-in information structure
 * @return #CP_OK (zero) on success or error code on failure
 */
CP_API int cp_install_plugin(cp_context_t *ctx, cp_plugin_info_t *pi);

/**
 * Scans for plug-ins in the registered plug-in directories, installing
 * new plug-ins and upgrading installed plug-ins. This function can be used to
 * initially load the plug-ins and to later rescan for new plug-ins.
 * 
 * When several versions of the same plug-in is available the most recent
 * version will be installed. The upgrade behavior depends on the specified
 * @ref scanFlags "flags". If #CP_LP_UPGRADE is set then upgrades to installed plug-ins are
 * allowed. The old version is unloaded and the new version installed instead.
 * If #CP_LP_STOP_ALL_ON_UPGRADE is set then all active plug-ins are stopped
 * if any plug-ins are to be upgraded. If #CP_LP_STOP_ALL_ON_INSTALL is set then
 * all active plug-ins are stopped if any plug-ins are to be installed or
 * upgraded. Finally, if #CP_LP_RESTART_ACTIVE is set all currently active
 * plug-ins will be restarted after the changes (if they were stopped).
 * 
 * When removing plug-in files from the plug-in directories, the
 * plug-ins to be removed must be first unloaded. Therefore this function
 * does not check for removed plug-ins.
 * 
 * @param ctx the plug-in context
 * @param flags the bitmask of flags
 * @return #CP_OK (zero) on success, an error code on failure
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
 * @return #CP_OK (zero) on success, an error code on failure
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
 * @return #CP_OK (zero) on success or #CP_ERR_UNKNOWN if unknown plug-in
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
 * Then uninstalls the plug-in and any dependent plug-ins.
 * 
 * @param ctx the plug-in context
 * @param id identifier of the plug-in to be unloaded
 * @return #CP_OK (zero) on success or #CP_ERR_UNKNOWN if unknown plug-in
 */
CP_API int cp_uninstall_plugin(cp_context_t *ctx, const char *id);

/**
 * Uninstalls all plug-ins. All plug-ins are first stopped and then
 * uninstalled.
 * 
 * @param ctx the plug-in context
 */
CP_API void cp_uninstall_all_plugins(cp_context_t *ctx);

/*@}*/


/**
 * @defgroup funcsPluginInfo Plug-in and extension information
 * @ingroup funcs
 *
 * These functions can be used to query information about the installed
 * plug-ins, extension points and extensions or to listen for plug-in state
 * changes. They may be used by the main program or by a plug-in runtime.
 */
/*@{*/

/**
 * Returns static information about the specified plug-in. The returned
 * information must not be modified and the caller must
 * release the information by calling ::cp_release_info when the
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
 * information must not be modified and the caller must
 * release the information by calling ::cp_release_info when the
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
 * The returned information must not be modified and the caller must
 * release the information by calling ::cp_release_info when the
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
 * The returned information must not be modified and the caller must
 * release the information by calling ::cp_release_info when the
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
 * #CP_PLUGIN_UNINSTALLED if the specified plug-in identifier is unknown.
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
 * @return #CP_OK (zero) on success, #CP_ERR_RESOURCE if out of resources
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
 * @defgroup funcsSymbols Dynamic symbols
 * @ingroup funcs
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
 * @return #CP_OK (zero) on success or an error code on failure
 */
CP_API int cp_define_symbol(cp_context_t *ctx, const char *name, void *ptr);

/**
 * Resolves a named symbol provided by the specified plug-in which is started
 * automatically if it is not already active. The symbol may
 * be context specific. The framework first looks for a context specific
 * symbol and then falls back to resolving a global symbol exported by the
 * plug-in runtime library. The plug-in framework creates dynamically a
 * dependency from the symbol using
 * plug-in to the symbol defining plug-in. The symbol can be released using
 * ::cp_release_symbol when it is not needed anymore. Unreleased
 * resolved symbols are released automatically after the resolving plug-in has
 * been stopped. Pointers to dynamically resolved symbols must not be passed on
 * to other plug-ins and the resolving plug-in must not use a pointer after
 * the symbol has been released.
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
 * ::cp_resolve_symbol for the same plug-in and symbol.
 *
 * @param ctx the plug-in context
 * @param ptr the pointer associated with the symbol
 */
CP_API void cp_release_symbol(cp_context_t *ctx, const void *ptr);

/*@}*/


#ifdef __cplusplus
}
#endif /*__cplusplus*/

#endif /*CPLUFF_H_*/