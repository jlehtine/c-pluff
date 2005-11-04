/*-------------------------------------------------------------------------
 * C-Pluff, a plug-in framework for C
 * Copyright 2005 Johannes Lehtinen
 *-----------------------------------------------------------------------*/

#ifndef _CPLUFF_H_
#define _CPLUFF_H_

#include <cpkazlib/list.h>

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/


/* ------------------------------------------------------------------------
 * Data types
 * ----------------------------------------------------------------------*/

/*
 * IMPORTANT NOTICE!
 * 
 * C-Pluff supports multi-threading (although it does not start new threads
 * on its own). External code accessing the global data structures of the
 * plug-in framework must use the provided locking functions (acquire and
 * release functions declared in this header file).
 */
 

/** Possible plug-in states */
typedef enum plugin_state_t {
	PLUGIN_UNINSTALLED,
	PLUGIN_INSTALLED,
	PLUGIN_RESOLVED,
	PLUGIN_STARTING,
	PLUGIN_STOPPING,
	PLUGIN_ACTIVE
} plugin_state_t;

/** Possible version match rules */
typedef enum version_match_t {
	MATCH_PERFECT,
	MATCH_EQUIVALENT,
	MATCH_COMPATIBLE,
	MATCH_GREATEROREQUAL
} version_match_t;

/** Event types */
typedef enum plugin_event_type_t {
	EVENT_PRECHANGE, /* committed to a state change */
	EVENT_POSTCHANGE /* state change complete */
} plugin_event_type_t;

/* Forward type definitions */
typedef struct plugin_t plugin_t;
typedef struct plugin_import_t plugin_import_t;
typedef struct ext_point_t ext_point_t;
typedef struct extension_t extension_t;
typedef struct plugin_list_t plugin_list_t;
typedef struct extension_list_t extension_list_t;


/* Plug-in data structures */

/**
 * Extension point structure captures information about a deployed extension
 * point.
 */
struct ext_point_t {
	
	/**
	 * Human-readable, possibly localized, extension point name or NULL
	 * if not available
	 */
	char *name;
	
	/** 
	 * Simple identifier uniquely identifying the extension point within the
	 * providing plug-in
	 */
	char *simpleIdentifier;
	
	/**
	 * Unique identifier constructed by concatenating the plug-in identifier,
	 * period character '.' and the simple identifier
	 */
	char *uniqueIdentifier;
	
	/** Plug-in providing this extension point */
	plugin_t *plugin;

	/** Extensions installed at this extension point */
	list_t *extensions;
	
};

/**
 * Extension structure captures information about a deployed extension.
 */
struct extension_t {
	
	/** 
	 * Human-readable, possibly localized, extension name or NULL if not
	 * available
	 **/
	char *name;
	
	/**
	 * Simple identifier uniquely identifying the extension within the
	 * providing plug-in or NULL if not available
	 */
	 char *simple_id;
	 
	/**
	 * Unique identifier constructed by concatenating the plug-in identifier,
	 * period character '.' and the simple identifier
	 */
	char *unique_id;
	  
	/** Plug-in providing this extension */
	plugin_t *plugin;

	/** Unique identifier of the extension point */
	char *extpt_id;
	 
	/** The extension point being extended, or NULL if not yet resolved */
	ext_point_t *extpt; 
	 
};

/**
 * Information about plug-in import.
 */
struct plugin_import_t {
	
	/** Identifier of the imported plug-in */
	char *identifier;
	
	/** Version to be matched, or NULL if none */
	char *version;
	
	/** Version match rule */
	version_match_t match;
	
	/** Whether this import is optional */
	int optional;
};

/**
 * Plug-in structure captures information about a deployed plug-in.
 */
struct plugin_t {
	
	/** State of this plug-in */
	plugin_state_t state;
	
	/** Human-readable, possibly localized, plug-in name */
	char *name;
	
	/** Unique identifier */
	char *identifier;
	
	/** Version string */
	char *versionString;
	
	/** Provider name, possibly localized */
	char *providerName;
	
	/** Number of imports */
	int num_imports;
	
	/** Imports */
	plugin_import_t *imports;

	/** Canonical path of the plugin directory */
	char *path;
	
	/** Imported plug-ins if plug-in has been resolved */
	plugin_list_t *imported;

	/** The start function, or NULL if none or plug-in is not resolved */
	int (*start_func)(void);
	
	/** The stop function, or NULL if none or plug-in is not resolved */
	void (*stop_func)(void);
	
	/** Number of extension points provided by this plug-in */
	int num_ext_points;
	
	/** Extension points provided by this plug-in */
	ext_point_t *ext_points;
	
	/** Number of extensions provided by this plugin */
	int num_extensions;
	
	/** Extensions provided by this plug-in */
	extension_t *extensions;
	
	/** Plug-ins currently importing this plug-in */
	list_t *importing;

};

/**
 * Describes a plug-in status event.
 */
typedef struct plugin_event_t {
	
	/** The associated plug-in */
	plugin_t *plugin;
	
	/**
	 * Type of the event. This tells whether the plug-in framework has
	 * committed to the change and about to perform it or whether the
	 * change is already complete.
	 */
	plugin_event_type_t type;
	
	/** Old state of the plug-in */
	plugin_state_t old_state;
	
	/** New state of the plug-in */
	plugin_state_t new_state;
	
} plugin_event_t;


/* ------------------------------------------------------------------------
 * External variables
 * ----------------------------------------------------------------------*/

/** Installed plug-ins */
list_t *plugins;


/* ------------------------------------------------------------------------
 * Function declarations
 * ----------------------------------------------------------------------*/


/* Initialization functions */

/**
 * Initializes the C-Pluff framework. This function must be called before
 * calling any other function or accessing C-Pluff data structures.
 */
void cpluff_init(void);


/* Functions for error handling */

/**
 * Adds an error handler function that will be called by the plug-in
 * framework when an error occurs. For example, failures to start and register
 * plug-ins are reported to the error handler function, if set. Error messages
 * are localized, if possible. There can be several registered error handlers.
 * 
 * @param error_handler the error handler to be added
 */
void add_cp_error_handler(void (*error_handler)(const char *msg));

/**
 * Removes an error handler.
 * 
 * @param error_handler the error handler to be removed
 */
void remove_cp_error_handler(void (*error_handler)(const char *msg));


/* Functions for registering plug-in event listeners */

/**
 * Adds an event listener which will be called on plug-in state changes.
 * The event listener is called synchronously after committing to but before
 * actually performing the actions related to a plug-in state change and
 * again immediately after the state change is complete. Installation and
 * uninstallation of a plugin are handled differently in that event listener is
 * only called after (for installation) or before (for uninstallation) the
 * actual state change. The read/write lock is being held while calling the
 * listener so the listener should return promptly. There can be several
 * registered listeners.
 * 
 * @param event_listener the event_listener to be added
 */
void add_cp_event_listener
	(void (*event_listener)(const plugin_event_t *event));

/**
 * Removes an event listener.
 * 
 * @param event_listener the event listener to be removed
 */
void remove_cp_event_listener
	(void (*event_listener)(const plugin_event_t *event));


/* Functions for finding plug-ins, extension points and extensions */

/**
 * Returns the plug-in having the specified identifier.
 * 
 * @param id the identifier of the plug-in
 * @return plug-in or NULL if no such plug-in exists
 */
plugin_t *find_plugin(const char *id);

/**
 * Returns the extension point having the specified identifier.
 * 
 * @param id the identifier of the extension point
 * @return extension point or NULL if no such extension point exists
 */
ext_point_t *find_ext_point(const char *id);

/**
 * Returns the extension having the specified identifier.
 * 
 * @param id the identifier of the extension
 * @return extension or NULL if no such extension exists
 */
extension_t *find_extension(const char *id);


/* Functions for controlling plug-ins */

/**
 * Scans for plug-ins in the specified directory and loads all plug-ins
 * found.
 * 
 * @param dir the directory containing plug-ins
 */
void scan_plugins(const char *dir);

/**
 * Loads a plug-in from a specified path. The plug-in is added to the list of
 * installed plug-ins. If the plug-in at the specified location has already
 * been loaded then a reference to the installed plug-in is
 * returned. If loading fails then NULL is returned.
 * 
 * @param path the installation path of the plug-in
 * @return reference to the plug-in, or NULL if loading fails
 */
plugin_t *load_plugin(const char *path);

/**
 * Starts a plug-in. The plug-in is first resolved, if necessary, and all
 * imported plug-ins are started. If the plug-in is already starting then
 * this function blocks until the plug-in has started or failed to start.
 * If the plug-in is already active then this function returns immediately.
 * If the plug-in is stopping then this function blocks until the plug-in
 * has stopped and then starts the plug-in.
 * 
 * @param plugin the plug-in to be started
 * @return whether the plug-in was successfully started
 */
int start_plugin(plugin_t *plugin);

/**
 * Stops a plug-in. First stops any importing plug-ins that are currently
 * active. Then stops the specified plug-in. If the plug-in is already
 * stopping then this function blocks until the plug-in has stopped. If the
 * plug-in is already stopped then this function returns immediately. If the
 * plug-in is starting then this function blocks until the plug-in has
 * started (or failed to start) and then stops the plug-in (or just returns
 * if the plug-in failed to start).
 * 
 * @param plugin the plug-in to be stopped
 */
void stop_plugin(plugin_t *plugin);

/**
 * Stops all active plug-ins.
 */
void stop_all_plugins(void);

/**
 * Unloads an installed plug-in. The plug-in is first stopped if it is active.
 * Then the importing plug-ins are unloaded
 * and finally the specified plug-in is unloaded.
 * 
 * @param plugin the plug-in to be unloaded
 */
void unload_plugin(plugin_t *plugin);

/**
 * Unloads all plug-ins. This effectively stops the plug-in framework and
 * releases the allocated resources.
 */
void unload_all_plugins(void);


/* Locking global data structures for exclusive or shared read-only access */

/**
 * Acquires shared read-only access for global C-Pluff data structures.
 */
void acquire_cp_data_ro();

/**
 * Releases shared read-only access for global C-Pluff data structures.
 */
void release_cp_data_ro();

/**
 * Acquires exclusive read/write access for global C-Pluff data structures.
 */
void acquire_cp_data_rw();

/**
 * Releases exclusive read/write access for global C-Pluff data structures.
 */
void release_cp_data_rw();


#ifdef __cplusplus
}
#endif /*__cplusplus*/

#endif /*_CPLUFF_H_*/
