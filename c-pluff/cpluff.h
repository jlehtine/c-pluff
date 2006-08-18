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
#elif __GNUC__ > 5 || (__GNUC__ == 5 && __GNUC_MINOR__ >= 3)
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
	int num_atts;
	
	/**
	 * Attribute name, value pairs (alternating, UTF-8 encoded)
	 */
	CP_API_CONST char * CP_API_CONST *atts;
	
	/** The value (text contents) of this configuration element (UTF-8) */
	CP_API_CONST char *value;
	
	/** The parent, or NULL if root node */
 	CP_API_CONST cp_cfg_element_t *parent;
 	
 	/** Number of children */
 	int num_children;

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
	
	/** Whether this import is optional */
	int optional;
};

/**
 * Plug-in structure captures information about a plug-in.
 */
struct cp_plugin_t {
	
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
	int num_imports;
	
	/** Imports */
	CP_API_CONST cp_plugin_import_t *imports;

    /** The relative path of plug-in runtime library, or empty if none */
    CP_API_CONST char *lib_path;
    
    /** The name of the start function, or empty if none */
    CP_API_CONST char *start_func_name;
    
    /** The name of the stop function, or empty if none */
    CP_API_CONST char *stop_func_name;

	/** Number of extension points provided by this plug-in */
	int num_ext_points;
	
	/** Extension points provided by this plug-in */
	CP_API_CONST cp_ext_point_t *ext_points;
	
	/** Number of extensions provided by this plugin */
	int num_extensions;
	
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
 * depends on the locale.
 */
typedef void (*cp_error_handler_t)(const char *msg);

/**
 * An event listener function called synchronously after a plugin state change.
 * An event listener function should return promptly and it must not register
 * or unregister event listeners.
 */
typedef void (*cp_event_listener_t)(const cp_plugin_event_t *event);

/**
 * A start function called to start a plug-in. The start function must return
 * non-zero on success and zero on failure. If the start fails then the
 * stop function (if any) is called to clean up plug-in state.
 */
typedef int (*cp_start_t)(void);

/**
 * A stop function called to stop a plugin.
 */
typedef void (*cp_stop_t)(void);


/* ------------------------------------------------------------------------
 * Function declarations
 * ----------------------------------------------------------------------*/


/* Initialization and destroy functions */

/**
 * Initializes the C-Pluff framework. The framework must be initialized before
 * trying to use it. This function does nothing if the framework has already
 * been initialized but the framework will be uninitialized only after the
 * corresponding number of calls to cpluff_destroy. This behavior is not
 * thread-safe, however.
 * 
 * @param error_handler an initial error handler, or NULL if none
 * @return CP_OK (0) on success, CP_ERR_RESOURCE if out of resources
 */
int CP_API cp_init(cp_error_handler_t error_handler);

/**
 * Stops and unloads all plug-ins and releases all resources allocated by
 * the C-Pluff framework. Framework functionality and data structures are not
 * available after calling this function. If cpluff_init has been called
 * multiple times then the actual uninitialization takes place only
 * after corresponding number of calls to cpluff_destroy. This behavior is not
 * thread-safe, however. The framework may be reinitialized by calling
 * cpluff_init function.
 */
void CP_API cp_destroy(void);


/* Functions for error handling */

/**
 * Adds an error handler function that will be called by the plug-in
 * framework when an error occurs. For example, failures to start and register
 * plug-ins are reported to the registered error handlers. Error messages are
 * localized, if possible. There can be several registered error handlers.
 * 
 * @param error_handler the error handler to be added
 * @return CP_OK (0) on success, CP_ERR_RESOURCE if out of resources
 */
int CP_API cp_add_error_handler(cp_error_handler_t error_handler);

/**
 * Removes an error handler. This function does nothing if the specified error
 * handler has not been registered.
 * 
 * @param error_handler the error handler to be removed
 */
void CP_API cp_remove_error_handler(cp_error_handler_t error_handler);


/* Functions for registering plug-in event listeners */

/**
 * Adds an event listener which will be called on plug-in state changes.
 * The event listener is called synchronously immediately after plug-in state
 * has changed. There can be several registered listeners.
 * 
 * @param event_listener the event_listener to be added
 * @return CP_OK (0) on success, CP_ERR_RESOURCE if out of resources
 */
int CP_API cp_add_event_listener(cp_event_listener_t event_listener);

/**
 * Removes an event listener. This function does nothing if the specified
 * listener has not been registered.
 * 
 * @param event_listener the event listener to be removed
 */
void CP_API cp_remove_event_listener(cp_event_listener_t event_listener);


/* Functions for controlling plug-ins */

/**
 * (Re)scans for plug-ins in the specified directory, re-installing updated
 * (and downgraded) plug-ins, installing new plug-ins and uninstalling plug-ins
 * that do not exist anymore. The allowed operations is specified as a bitmask.
 * This method can also be used to initially load the plug-ins.
 * 
 * @param dir the directory containing plug-ins
 * @param flags a bitmask specifying allowed operations (CPLUFF_RESCAN_...)
 * @return CP_OK (0) on success, an error code on failure
 */
int CP_API cp_rescan_plugins(const char *dir, int flags);

/**
 * Loads a plug-in from the specified path. The plug-in is added to the list of
 * installed plug-ins. If loading fails then NULL is returned.
 * 
 * @param path the installation path of the plug-in
 * @param id the pointer to the identifier of the loaded plug-in is copied to
 *     the location pointed to by this pointer if this pointer is non-NULL
 * @return CP_OK (0) on success, an error code on failure
 */
int CP_API cp_load_plugin(const char *path, char **id);

/**
 * Starts a plug-in. The plug-in is first resolved, if necessary, and all
 * imported plug-ins are started. If the plug-in is already starting then
 * this function blocks until the plug-in has started or failed to start.
 * If the plug-in is already active then this function returns immediately.
 * If the plug-in is stopping then this function blocks until the plug-in
 * has stopped and then starts the plug-in.
 * 
 * @param id identifier of the plug-in to be started
 * @return CP_OK (0) on success, an error code on failure
 */
int CP_API cp_start_plugin(const char *id);

/**
 * Stops a plug-in. First stops any importing plug-ins that are currently
 * active. Then stops the specified plug-in. If the plug-in is already
 * stopping then this function blocks until the plug-in has stopped. If the
 * plug-in is already stopped then this function returns immediately. If the
 * plug-in is starting then this function blocks until the plug-in has
 * started (or failed to start) and then stops the plug-in.
 * 
 * @param id identifier of the plug-in to be stopped
 * @return CP_OK (0) on success, an error code on failure
 */
int CP_API cp_stop_plugin(const char *id);

/**
 * Stops all active plug-ins.
 */
void CP_API cp_stop_all_plugins(void);

/**
 * Unloads a plug-in. The plug-in is first stopped if it is active.
 * 
 * @param id identifier of the plug-in to be unloaded
 * @return CP_OK (0) on success, an error code on failure
 */
int CP_API cp_unload_plugin(const char *id);

/**
 * Unloads all plug-ins. This effectively stops all plug-in activity and
 * releases the resources allocated by the plug-ins.
 */
void CP_API cp_unload_all_plugins(void);


/* Functions for accessing plug-ins */

/**
 * Returns static information about the specified plug-in. The caller must
 * release the information by calling cp_release_plugin_info as soon as
 * it does not need the information anymore.
 * 
 * @param id identifier of the plug-in to be examined
 * @param error filled with an error code, if non-NULL
 * @return pointer to the information structure or NULL if error occurs
 */
const cp_plugin_t * CP_API cp_get_plugin(const char *id, int *error);

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
