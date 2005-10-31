/*-------------------------------------------------------------------------
 * C-Pluff, a plug-in framework for C
 * Copyright 2005 Johannes Lehtinen
 *-----------------------------------------------------------------------*/

#ifndef _CPLUFF_H_
#define _CPLUFF_H_

/* Plug-in framework API version */
#define CPLUFF_API_VERSION 1


/* ------------------------------------------------------------------------
 * Data structures and constants
 * ----------------------------------------------------------------------*/

/* 
 * Plug-in information structure.
 */
struct cpluff_plugin {

	/* 
	 * The C-Pluff API version used by the plug-in implementation. This
	 * should be the value of CPLUFF_API_VERSION at the time this plug-in was
	 * compiled. It will be checked against the API version supported by the
	 * application.
	 */
	int cpluff_api_version;
	
	/*
	 * The function called to start the plugin, or NULL if none. The function
	 * returns 0 on success and 1 on failure. This function is called after
	 * this plug-in has been loaded and all the plug-ins this plug-in depends
	 * on have been successfully started.
	 */
	int (*start)();
	
	/*
	 * The function called to stop the plugin, or NULL if none. This function
	 * is called before unloading this plug-in or stopping any of the plug-ins
	 * this plugin depends on.
	 */
	void (*stop)();
	
};
typedef struct cpluff_plugin cpluff_plugin;


/* ------------------------------------------------------------------------
 * Function declarations
 * ----------------------------------------------------------------------*/

#endif /*_CPLUFF_H_*/
