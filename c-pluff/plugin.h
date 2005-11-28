/*-------------------------------------------------------------------------
 * C-Pluff, a plug-in framework for C
 * Copyright 2005 Johannes Lehtinen
 *-----------------------------------------------------------------------*/

#ifndef _PLUGIN_H_
#define _PLUGIN_H_

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/


/* ------------------------------------------------------------------------
 * Constants
 * ----------------------------------------------------------------------*/

/**
 * Maximum length of a plug-in, extension or extension point name in bytes,
 * excluding the trailing '\0'
 */
#define CP_NAME_MAX_LENGTH 127

/**
 * Maximum length of a version string in bytes, excluding the trailing '\0'
 */
#define CP_VERSTR_MAX_LENGTH 31

/** Preliminarily OK */
#define CP_OK_PRELIMINARY 1


/* ------------------------------------------------------------------------
 * Data types
 * ----------------------------------------------------------------------*/

/** A component name in UTF-8 encoding */
typedef char cp_name_t[CP_NAME_MAX_LENGTH + 1];

/** A plug-in version string */
typedef char cp_verstr_t[CP_VERSTR_MAX_LENGTH + 1];

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
typedef struct cp_extension_t cp_extension_t;
typedef struct cp_plugin_t cp_plugin_t;


/* Plug-in data structures */

/**
 * Extension point structure captures information about an extension
 * point.
 */
struct cp_ext_point_t {
	
	/**
	 * Human-readable, possibly localized, extension point name or empty
	 * if not available
	 */
	cp_name_t name;
	
	/**
	 * Simple identifier uniquely identifying the extension point within the
	 * providing plug-in
	 */
	cp_id_t simple_id;
	
};

/**
 * Extension structure captures information about an extension.
 */
struct cp_extension_t {
	
	/** 
	 * Human-readable, possibly localized, extension name or empty if not
	 * available
	 **/
	cp_name_t name;
	
	/**
	 * Simple identifier uniquely identifying the extension within the
	 * providing plug-in or empty if not available
	 */
	cp_id_t simple_id;
	 
	/** Unique identifier of the extension point */
	cp_id_t extpt_id;

};

/**
 * Information about plug-in import.
 */
struct cp_plugin_import_t {
	
	/** Identifier of the imported plug-in */
	cp_id_t plugin_id;
	
	/** Version to be matched, or empty if none */
	cp_verstr_t version;
	
	/** Version match rule */
	cp_version_match_t match;
	
	/** Whether this import is optional */
	int optional;
};

/**
 * Plug-in structure captures information about a plug-in.
 */
struct cp_plugin_t {
	
	/** Human-readable, possibly localized, plug-in name */
	cp_name_t name;
	
	/** Unique identifier */
	cp_id_t identifier;
	
	/** Version string */
	cp_verstr_t version;
	
	/** Provider name, possibly localized */
	cp_name_t provider_name;
	
	/** Absolute path of the plugin directory, or NULL if not known */
	char *path;
	
	/** Number of imports */
	int num_imports;
	
	/** Imports */
	cp_plugin_import_t *imports;

    /** The relative path of plug-in runtime library, or empty if none */
    char lib_path[128];
    
    /** The name of the start function, or empty if none */
    char start_func_name[32];
    
    /** The name of the stop function, or empty if none */
    char stop_func_name[32];

	/** Number of extension points provided by this plug-in */
	int num_ext_points;
	
	/** Extension points provided by this plug-in */
	cp_ext_point_t *ext_points;
	
	/** Number of extensions provided by this plugin */
	int num_extensions;
	
	/** Extensions provided by this plug-in */
	cp_extension_t *extensions;

};


/* ------------------------------------------------------------------------
 * Function declarations
 * ----------------------------------------------------------------------*/

/**
 * Initializes the plug-in controlling component.
 * 
 * @return CP_OK (0) on success, an error code on failure
 */
int cpi_init_plugins(void);

/**
 * Destroys the plug-in controlling component.
 */
void cpi_destroy_plugins(void);


#ifdef __cplusplus
}
#endif /*__cplusplus */

#endif /*_PLUGIN_H_*/
