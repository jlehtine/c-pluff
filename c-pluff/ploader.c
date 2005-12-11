/*-------------------------------------------------------------------------
 * C-Pluff, a plug-in framework for C
 * Copyright 2005 Johannes Lehtinen
 *-----------------------------------------------------------------------*/

/*
 * Plug-in loading functionality
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <expat.h>
#include "cpluff.h"
#include "pcontrol.h"


/* ------------------------------------------------------------------------
 * Constants
 * ----------------------------------------------------------------------*/

/* Path separator characters to be used */

#ifdef __WIN32__

#define CP_PATHSEP_CHAR '\\'
#define CP_PATHSEP_STR "\\"

#else /*__WIN32__*/

#define CP_PATHSEP_CHAR '/'
#define CP_PATHSEP_STR "/"

#endif /*__WIN32__*/


/* ------------------------------------------------------------------------
 * Static function declarations
 * ----------------------------------------------------------------------*/

/**
 * Loads plug-in from the specified file.
 * 
 * @param file the plug-in descriptor file
 * @param plugin where to store the pointer to the newly allocated plug-in
 * @return CP_OK (0) on success, an error code on failure
 */
static int load_plugin(const char *file, cp_plugin_t **plugin);

/* ------------------------------------------------------------------------
 * Function definitions
 * ----------------------------------------------------------------------*/

/* Plug-in loading */

CP_API(int) cp_rescan_plugins(const char *dir, int flags) {
	assert(dir != NULL);
	/* TODO */
	return CP_ERR_UNSPECIFIED;
}

CP_API(int) cp_load_plugin(const char *path, cp_id_t *id) {
	cp_plugin_t *plugin = NULL;
	int status;

	assert(path != NULL);
	do {

		/* Load the plug-in */
		status = load_plugin(path, &plugin);
		if (status != CP_OK) {
			break;
		}

		/* Register the plug-in */
		status = cpi_install_plugin(plugin);
		if (status != CP_OK) {
			break;
		}
	
	} while (0);
	
	/* Release any allocated data on failure */
	if (status != CP_OK) {
		if (plugin != NULL) {
			cpi_free_plugin(plugin);
		}
	}
	
	return status;
}

static int load_plugin(const char *path, cp_plugin_t **plugin) {
	cp_plugin_t *p = NULL;
	char *file = NULL;
	const char *postfix = CP_PATHSEP_STR "plugin.xml";
	int status = CP_OK;
	int fd = -1;
/*	XMLParser parser = NULL;*/

	do {
		int i;

		/* Construct the file name for the plug-in descriptor */
		i = strlen(path);
		assert(i > 0);
		file = malloc((i + strlen(postfix) + 1) * sizeof(char));
		if (file == NULL) {
			status = CP_ERR_RESOURCE;
			break;
		}
		strcpy(file, path);
		if (i > 1 && path[i-1] == CP_PATHSEP_CHAR) {
			i--;
		}
		strcpy(file + i, postfix);

		/* Open the file */
		{
			int mode = O_RDONLY;
#ifdef O_BINARY
			mode |= O_BINARY;
#endif
#ifdef O_SEQUENTIAL
			mode |= O_SEQUENTIAL;
#endif
			fd = open(file, mode);
			if (fd == -1) {
				status = CP_ERR_IO;
				break;
			}
		}

		/* Initialize the XML parsing */
		/*parser = XML_ParserCreate(NULL);*/

		/* TODO */
		
		/* Close the file */
		close(fd);
		fd = -1;

		/* Initialize the plug-in path */
		*(file + i) = '\0';
		p->path = file;
		
	} while (0);
		
	/* Release any allocated data on failure */
	if (status != CP_OK) {
/*		if (parser != NULL) {
		}*/
		if (fd != -1) {
			close(fd);
		}
		if (file != NULL && (p == NULL || p->path == NULL)) {
			free(file);
		}
		if (p != NULL) {
			cpi_free_plugin(p);
		}
	}

	/* TODO */
	return CP_ERR_UNSPECIFIED;
}
