/*-------------------------------------------------------------------------
 * C-Pluff, a plug-in framework for C
 * Copyright 2005-2006 Johannes Lehtinen
 *-----------------------------------------------------------------------*/

/*
 * The public API header file
 */

#ifndef _CPLUFF_H_
#define _CPLUFF_H_

/* Define CP_API to declare API functions */
#ifdef _MSC_EXTENSIONS
#ifdef CP_BUILD
#define CP_API __declspec(dllexport)
#else /*CP_BUILD*/
#define CP_API __declspec(dllimport)
#endif /*CP_BUILD*/
#elif __GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 3)
#define CP_API __attribute__ ((visibility ("default")))
#else /* Not restricting link time visibility of non-API symbols */
#define CP_API
#endif

/* Define CP_API_CONST to declare const API data (outside implementation) */
#ifdef CP_BUILD
#define CP_API_CONST
#else /*CP_BUILD*/
#define CP_API_CONST const
#endif /*CP_BUILD*/

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/


/* ------------------------------------------------------------------------
 * Constants
 * ----------------------------------------------------------------------*/

/* Error codes for operations that might fail */

/** Operation performed successfully */
#define CP_OK 0

/** An unspecified error occurred */
#define CP_ERR_UNSPECIFIED 1

/** Not enough memory or other OS resources available */
#define CP_ERR_RESOURCE 2

/** The specified object is unknown to the framework */
#define CP_ERR_UNKNOWN 3

/** An I/O error occurred */
#define CP_ERR_IO 4

/** Malformed plug-in data when loading a plug-in */
#define CP_ERR_MALFORMED 5

/** Plug-in conflicts with an existing plug-in when loading a plug-in */
#define CP_ERR_CONFLICT 6

/** Plug-in dependencies could not be satisfied */
#define CP_ERR_DEPENDENCY 7

/** An error in a plug-in runtime */
#define CP_ERR_RUNTIME 8

/** A blocking operation failed to prevent a deadlock */
#define CP_ERR_DEADLOCK 9


/* Flags for cp_rescan_plugins */

/** Setting this flag enables installation of new plug-ins */
#define CP_RESCAN_INSTALL 0x01

/** Settings this flag enables upgrades of installed plug-ins */
#define CP_RESCAN_UPGRADE 0x02

/** Setting this flag enables downgrades of installed plug-ins */
#define CP_RESCAN_DOWNGRADE 0x04

/** Setting this flag enables uninstallation of removed plug-ins */
#define CP_RESCAN_UNINSTALL 0x08

/**
 * Setting this flag causes all services to be stopped if there are any
 * changes to the currently installed plug-ins
 */
#define CP_RESCAN_STOP_ALL 0x10

/**
 * Setting this flag causes the currently active plug-ins to be restarted
 * (if they were stopped) after all changes to the plug-ins have been made
 */
#define CP_RESCAN_RESTART_ACTIVE 0x20

/**
 * This bitmask corresponds to incremental loading of plug-ins. New plug-ins
 * are installed and installed ones are upgraded if possible but no plug-ins
 * are uninstalled or downgraded.
 */
#define CP_RESCAN_INCREMENTAL 0x03

/** This bitmask corresponds to full rescan */
#define CP_RESCAN_FULL 0x0f

/** This bitmask corresponds to full rescan and full restart */
#define CP_RESCAN_FULL_RESTART 0x3f


/* ------------------------------------------------------------------------
 * Data types
 * ----------------------------------------------------------------------*/

/** Possible version match rules */
typedef enum cp_version_match_t {
	CP_MATCH_NONE,
	CP_MATCH_PERFECT,
	CP_MATCH_EQUIVALENT,
	CP_MATCH_COMPATIBLE,
	CP_MATCH_GREATEROREQUAL
} cp_version_match_t;

/* Forward type definitions */
typedef struct cp_plugin_import_t cp_plugin_import_t;
typedef struct cp_ext_point_t cp_ext_point_t;
typedef struct cp_cfg_element_t cp_cfg_element_t;
typedef struct cp_extension_t cp_extension_t;
typedef struct cp_plugin_t cp_plugin_t;
typedef struct cp_context_t cp_context_t;


/* Plug-in data structures */

/**
 * Extension point structure captures information about an extension
 * point.
 */
struct cp_ext_point_t {
	
	/**
	 * Human-readable, possibly localized, extension point name or NULL
	 * if not available (UTF-8)
	 */
	CP_API_CONST char *name;
	
	/**
	 * Local identifier uniquely identifying the extension point within the
	 * providing plug-in (UTF-8)
	 */
	CP_API_CONST char *local_id;
	
	/** Unique identifier of the extension point (UTF-8) */
	CP_API_CONST char *global_id;

	/** Path to the extension schema definition or NULL if none. */
	CP_API_CONST char *schema_path;
};

/**
 * Configuration element contains configuration information for an extension.
 */
struct cp_cfg_element_t {
	
	/** Name of the configuration element (UTF-8) */
	CP_API_CONST char *name;

	/** Number of attributes */
	unsigned int num_atts;
	
	/**
	 * Attribute name, value pairs (alternating, UTF-8 encoded)
	 */
	CP_API_CONST char * CP_API_CONST *atts;
	
	/** The value (text contents) of this configuration element (UTF-8) */
	CP_API_CONST char *value;
	
	/** The parent, or NULL if root node */
 	CP_API_CONST cp_cfg_element_t *parent;
 	
 	/** The index among siblings (0-based) */
 	unsigned int index;
 	
 	/** Number of children */
 	unsigned int num_children;

	/** Children */
	CP_API_CONST cp_cfg_element_t *children;
};

/**
 * Extension structure captures information about an extension.
 */
struct cp_extension_t {
	
	/** 
	 * Human-readable, possibly localized, extension name or NULL if not
	 * available (UTF-8)
	 **/
	CP_API_CONST char *name;
	
	/**
	 * Local identifier uniquely identifying the extension within the
	 * providing plug-in or NULL if not available (UTF-8)
	 */
	CP_API_CONST char *local_id;

    /** Unique identifier of the extension or NULL if not available (UTF-8) */
    CP_API_CONST char *global_id;
	 
	/** Unique identifier of the extension point (UTF-8) */
	CP_API_CONST char *ext_point_id;
	
	/** Extension configuration (starting with the extension element) */
	CP_API_CONST cp_cfg_element_t *configuration;
};

/**
 * Information about plug-in import.
 */
struct cp_plugin_import_t {
	
	/** Identifier of the imported plug-in (UTF-8) */
	CP_API_CONST char *plugin_id;
	
	/** Version to be matched, or NULL if none (UTF-8) */
	CP_API_CONST char *version;
	
	/** Version match rule */
	cp_version_match_t match;
	
	/** Whether this import is optional (1 for optional, 0 for mandatory) */
	int optional;
};

/**
 * Plug-in structure captures information about a plug-in.
 */
struct cp_plugin_t {
	
	/** The associated plug-in context (loading context) */
	CP_API_CONST cp_context_t *context;
	
	/** Human-readable, possibly localized, plug-in name (UTF-8) */
	CP_API_CONST char *name;
	
	/** Unique identifier (UTF-8) */
	CP_API_CONST char *identifier;
	
	/** Version string (UTF-8) */
	CP_API_CONST char *version;
	
	/** Provider name, possibly localized (UTF-8) */
	CP_API_CONST char *provider_name;
	
	/** Absolute path of the plugin directory, or NULL if not known */
	CP_API_CONST char *path;
	
	/** Number of imports */
	unsigned int num_imports;
	
	/** Imports */
	CP_API_CONST cp_plugin_import_t *imports;

    /** The relative path of plug-in runtime library, or empty if none */
    CP_API_CONST char *lib_path;
    
    /** The name of the start function, or empty if none */
    CP_API_CONST char *start_func_name;
    
    /** The name of the stop function, or empty if none */
    CP_API_CONST char *stop_func_name;

	/** Number of extension points provided by this plug-in */
	unsigned int num_ext_points;
	
	/** Extension points provided by this plug-in */
	CP_API_CONST cp_ext_point_t *ext_points;
	
	/** Number of extensions provided by this plugin */
	unsigned int num_extensions;
	
	/** Extensions provided by this plug-in */
	CP_API_CONST cp_extension_t *extensions;

};


/* Plug-in event data structures */

/** Possible plug-in states */
typedef enum cp_plugin_state_t {
	CP_PLUGIN_UNINSTALLED,
	CP_PLUGIN_INSTALLED,
	CP_PLUGIN_RESOLVED,
	CP_PLUGIN_STARTING,
	CP_PLUGIN_STOPPING,
	CP_PLUGIN_ACTIVE
} cp_plugin_state_t;

/**
 * Describes a plug-in status event.
 */
typedef struct cp_plugin_event_t {
	
	/** The identifier of the affected plug-in in UTF-8 encoding */
	CP_API_CONST char *plugin_id;
	
	/** Old state of the plug-in */
	cp_plugin_state_t old_state;
	
	/** New state of the plug-in */
	cp_plugin_state_t new_state;
	
} cp_plugin_event_t;


/* Prototypes for function pointers */

/** 
 * An error handler function called when a recoverable error occurs. An error
 * handler function should return promptly and it must not register or
 * unregister error handlers. The error message is localized and the encoding
 * depends on the locale. Attempts at changing plug-in state within the error
 * handler function may fail to prevent deadlocks.
 * 
 * @param context the associated plug-in context (NULL during context creation)
 * @param msg the localized error message
 */
typedef void (*cp_error_handler_t)(cp_context_t *context, const char *msg);

/**
 * An event listener function called synchronously after a plugin state change.
 * An event listener function should return promptly and it must not register
 * or unregister event listeners. Attempts at changing plug-in state within the event
 * listener function may fail to prevent deadlocks.
 * 
 * @param context the associated plug-in context
 * @param event te plug-in state change event
 */
typedef void (*cp_event_listener_t)(cp_context_t *context, const cp_plugin_event_t *event);

/**
 * A start function called to start a plug-in. The start function must return
 * non-zero on success and zero on failure. If the start fails then the
 * stop function (if any) is called to clean up plug-in state. Attempts at changing
 * plug-in state within the start function may fail to prevent deadlocks.
 * 
 * @param context the associated plug-in context
 * @return non-zero on success, or zero on failure
 */
typedef int (*cp_start_t)(cp_context_t *context);

/**
 * A stop function called to stop a plugin. Attempts at changing
 * plug-in state within the stop function may fail to prevent deadlocks.
 * 
 * @param context the associated plug-in context
 */
typedef void (*cp_stop_t)(cp_context_t *context);


/* ------------------------------------------------------------------------
 * Function declarations
 * ----------------------------------------------------------------------*/


/* Initialization and destroy functions */

/**
 * Creates a new plug-in context which is an instance of the C-Pluff plug-in framework.
 * Plug-ins are loaded and installed into a specific context. An application
 * may have more than one plug-in context but the plug-ins that interact with
 * each other should be placed in the same context. The resources associated with
 * the context are released by calling cp_destroy_context when the context is not
 * needed anymore.
 * 
 * @param error_handler an initial error handler, or NULL if none
 * @param error pointer to the location where error code or CP_OK is stored, or NULL
 * @return the newly created plugin context, or NULL on failure
 */
cp_context_t * CP_API cp_create_context(cp_error_handler_t error_handler, int *error);

/**
 * Destroys the specified plug-in context and releases the associated resources.
 * Stops and unloads all plug-ins in the context. The context and the associated
 * data structures such as plug-in information must not be accessed after calling
 * this function.
 * 
 * @param context the context to be destroyed
 */
void CP_API cp_destroy_context(cp_context_t *context);


/* Functions for error handling */

/**
 * Registers an error handler function with a plug-in context. The handler is called
 * synchronously when an error occurs. For example, failures to start and register
 * plug-ins are reported to the registered error handlers. There can be several
 * error handlers registered with the same context.
 * 
 * @param context the plug-in context
 * @param error_handler the error handler to be added
 * @return CP_OK (0) on success, CP_ERR_RESOURCE if out of resources, or CP_ERR_DEADLOCK
 * 		if called from an error handler
 */
int CP_API cp_add_error_handler(cp_context_t *context, cp_error_handler_t error_handler);

/**
 * Removes an error handler from a plug-in context.
 * 
 * @param context the plug-in context
 * @param error_handler the error handler to be removed
 * @return CP_OK (0) on success, CP_ERR_UNKNOWN if the error handler is unknown,
 * 		or CP_ERR_DEADLOCK if called from an error handler
 */
int CP_API cp_remove_error_handler(cp_context_t *context, cp_error_handler_t error_handler);


/* Functions for registering plug-in event listeners */

/**
 * Registers an event listener with a plug-in context. The event listener is called
 * synchronously immediately after a plug-in state change. There can be several
 * listeners registered with the same context.
 * 
 * @param context the plug-in context
 * @param event_listener the event_listener to be added
 * @return CP_OK (0) on success, CP_ERR_RESOURCE if out of resources, or
 * 		CP_ERR_DEADLOCK if called from an event listener
 */
int CP_API cp_add_event_listener(cp_context_t *context, cp_event_listener_t event_listener);

/**
 * Removes an event listener from a plug-in context.
 * 
 * @param context the plug-in context
 * @param event_listener the event listener to be removed
 * @return CP_OK (0) on success, CP_ERR_UNKNOWN if the event listener is unknown,
 * 		or CP_ERR_DEADLOCK if called from an error handler
 */
int CP_API cp_remove_event_listener(cp_context_t *context, cp_event_listener_t event_listener);


/* Functions for configuring a plug-in context */

/**
 * Registers a directory of plug-ins with a plug-in context. The
 * plug-in context will scan the directory when cp_rescan_plugins is called.
 * 
 * @param context the plug-in context
 * @param dir the directory
 * @return CP_OK (0) on success, or CP_ERR_RESOURCE if out of resources
 */
int CP_API cp_add_plugin_dir(cp_context_t *context, const char *dir);

/**
 * Unregisters a previously registered directory of plug-ins from a plug-in context.
 * Does not delete the directory itself.
 * 
 * @param context the plug-in context
 * @param dir the previously registered directory
 * @param unload whether the plug-ins loaded from this directory should be unloaded
 * @return CP_OK (0) on success, or error code on error
 */
int CP_API cp_remove_plugin_dir(cp_context_t *context, const char *dir, int unload);

/**
 * Sets the plug-in directories for the plug-in context. The plug-in directories
 * will be scanned when cp_rescan_plugins is called.
 * 
 * @param context the plug-in context
 * @param dirs NULL-terminated array of directories
 * @param unload whether plug-ins loaded from the removed directories should be unloaded
 * @return CP_OK (0) on success, or error code on error
 */
int CP_API cp_set_plugin_dirs(cp_context_t *context, const char * const *dirs, int unload);


/* Functions for controlling plug-ins */

/**
 * (Re)scans for plug-ins in the registered plug-in directories, re-installing updated
 * (and downgraded) plug-ins, installing new plug-ins and uninstalling plug-ins
 * that do not exist anymore. The allowed operations are specified as a bitmask.
 * This method can also be used to initially load the plug-ins.
 * 
 * @param context the plug-in context
 * @param flags a bitmask specifying allowed operations (CPLUFF_RESCAN_...)
 * @return CP_OK (0) on success, an error code on failure
 */
int CP_API cp_rescan_plugins(cp_context_t *context, int flags);

/**
 * Loads a plug-in from the specified path and returns static information about
 * the loaded plug-in. The plug-in is installed to the specified plug-in context.
 * If loading fails then NULL is returned. The caller must release the returned
 * information by calling cp_release_plugin_info as soon as it does not need
 * the information anymore.
 * 
 * @param context the plug-in context
 * @param path the installation path of the plug-in
 * @param error pointer to the location for the returned error code, or NULL
 * @return pointer to the information structure or NULL if error occurs
 */
const cp_plugin_t * CP_API cp_load_plugin(cp_context_t *context, const char *path, int *error);

/**
 * Starts a plug-in. The plug-in is first resolved, if necessary, and all
 * imported plug-ins are started. If the plug-in is already starting then
 * this function blocks until the plug-in has started or failed to start.
 * If the plug-in is already active then this function returns immediately.
 * If the plug-in is stopping then this function blocks until the plug-in
 * has stopped and then starts the plug-in.
 * 
 * @param context the plug-in context
 * @param id identifier of the plug-in to be started
 * @return CP_OK (0) on success, an error code on failure
 */
int CP_API cp_start_plugin(cp_context_t *context, const char *id);

/**
 * Stops a plug-in. First stops any importing plug-ins that are currently
 * active. Then stops the specified plug-in. If the plug-in is already
 * stopping then this function blocks until the plug-in has stopped. If the
 * plug-in is already stopped then this function returns immediately. If the
 * plug-in is starting then this function blocks until the plug-in has
 * started (or failed to start) and then stops the plug-in.
 * 
 * @param context the plug-in context
 * @param id identifier of the plug-in to be stopped
 * @return CP_OK (0) on success, an error code on failure
 */
int CP_API cp_stop_plugin(cp_context_t *context, const char *id);

/**
 * Stops all active plug-ins.
 * 
 * @param context the plug-in context
 */
void CP_API cp_stop_all_plugins(cp_context_t *context);

/**
 * Unloads a plug-in. The plug-in is first stopped if it is active.
 * 
 * @param context the plug-in context
 * @param id identifier of the plug-in to be unloaded
 * @return CP_OK (0) on success, an error code on failure
 */
int CP_API cp_unload_plugin(cp_context_t *context, const char *id);

/**
 * Unloads all plug-ins. This effectively stops all plug-in activity and
 * releases the resources allocated by the plug-ins.
 * 
 * @param context the plug-in context
 */
void CP_API cp_unload_all_plugins(cp_context_t *context);


/* Functions for accessing plug-ins */

/**
 * Returns static information about the specified plug-in. The caller must
 * release the information by calling cp_release_plugin_info as soon as
 * it does not need the information anymore.
 * 
 * @param context the plug-in context
 * @param id identifier of the plug-in to be examined
 * @param error filled with an error code, if non-NULL
 * @return pointer to the information structure or NULL if error occurs
 */
const cp_plugin_t * CP_API cp_get_plugin(cp_context_t *context, const char *id, int *error);

/**
 * Releases a previously obtained plug-in information structure.
 * 
 * @param plugin the plug-in information structure to be released
 */
void CP_API cp_release_plugin(const cp_plugin_t *plugin);


#ifdef __cplusplus
} /*extern "C"*/
#endif /*__cplusplus*/

#endif /*_CPLUFF_H_*/
