/*-------------------------------------------------------------------------
 * C-Pluff, a plug-in framework for C
 * Copyright 2005-2006 Johannes Lehtinen
 *-----------------------------------------------------------------------*/

/*
 * Plug-in loading functionality
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <expat.h>
#include "cpluff.h"
#include "core.h"
#include "pcontrol.h"
#include "kazlib/list.h"


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

/** XML parser buffer size (in bytes) */
#define CP_XML_PARSER_BUFFER_SIZE 4096

/* ------------------------------------------------------------------------
 * Internal data types
 * ----------------------------------------------------------------------*/

typedef struct ploader_context_t ploader_context_t;

/** Parser states */
typedef enum parser_state_t {
	PARSER_BEGIN,
	PARSER_PLUGIN,
	PARSER_REQUIRES,
	PARSER_EXTENSION,
	PARSER_END,
	PARSER_UNKNOWN,
	PARSER_ERROR
} parser_state_t;

/** Plug-in loader context */
struct ploader_context_t {

	/** The XML parser being used */
	XML_Parser parser;
	
	/** The file being parsed */
	const char *file;
	
	/** The plug-in being constructed */
	cp_plugin_t *plugin;
	
	/** The current parser state */
	parser_state_t state;
	
	/** The saved parser state (used in PARSER_UNKNOWN) */
	parser_state_t saved_state;
	
	/** The current parser depth (used in PARSER_UNKNOWN) */
	int depth;
	
	/** The list of imports for the plug-in being loaded */
	list_t *imports;

	/** The number of parsing errors that have occurred */
	int error_count;
	
	/** The number of resource errors that have occurred */
	int resource_error_count;
};

/* ------------------------------------------------------------------------
 * Static function declarations
 * ----------------------------------------------------------------------*/

/**
 * Loads a plug-in from the specified path.
 * 
 * @param path the plug-in path
 * @param plugin where to store the pointer to the loaded plug-in
 * @return CP_OK (0) on success, an error code on failure
 */
static int load_plugin(const char *path, cp_plugin_t **plugin);

/**
 * Processes the start of element events while parsing.
 * 
 * @param context the parsing context
 * @param name the element name
 * @param atts the element attributes
 */
static void XMLCALL cp_start_element_handler(
	void *context, const XML_Char *name, const XML_Char **atts);

/**
 * Processes the end of element events while parsing.
 * 
 * @param context the parsing context
 * @param name the element name
 */
static void XMLCALL cp_end_element_handler(
	void *context, const XML_Char *name);

/**
 * Puts the parser to a state in which it skips an unknown element.
 * Warns error handlers about the unknown element.
 * 
 * @param context the parsing context
 * @param elem the element name
 */
static void unexpected_element(ploader_context_t *context, const XML_Char *elem);

/**
 * Checks that an element has non-empty values for required attributes and
 * warns if there are unknown attributes. Increments the error count for
 * each missing required attribute.
 * 
 * @param context the parsing context
 * @param atts the attribute list for the element
 * @param req_atts the required attributes (NULL terminated list, or NULL)
 * @param opt_atts the optional attributes (NULL terminated list, or NULL)
 * @return whether the required attributes are present
 */
static int check_attributes(ploader_context_t *context, const XML_Char **atts,
	const XML_Char **req_atts, const XML_Char **opt_atts);

/**
 * Reports a descriptor error. Does not set the parser to error state but
 * increments the error count, unless this is merely a warning.
 * 
 * @param context the parsing context
 * @param warn whether this is only a warning
 * @param error_msg the error message
 * @param ... parameters for the error message
 */
static void descriptor_errorf(ploader_context_t *context, int warn,
	const char *error_msg, ...) CP_PRINTF(3, 4);

/**
 * Reports insufficient resources while parsing and increments the
 * resource error count.
 * 
 * @param context the parsing context
 */
static void resource_error(ploader_context_t *context);

/**
 * Returns whether the specified NULL-terminated list of strings includes
 * the specified string.
 * 
 * @param list the NULL-terminated list of strings, or NULL if none
 * @param str the string
 * @param step the stepping (1 to check every string or 2 to check every
 * 			other string)
 * @return pointer to the location of the string or NULL if not found
 */
static const XML_Char **contains_str(const XML_Char **list,
	const XML_Char *str, int step) CP_PURE;

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
	
	/* On success, return the plug-in id */
	if (status == CP_OK) {
		strcpy((char *) id, plugin->identifier);
	}
	
	/* Release any allocated data on failure */
	else {
		if (plugin != NULL) {
			cpi_free_plugin(plugin);
		}
	}
	
	return status;
}

static int load_plugin(const char *path, cp_plugin_t **plugin) {
	char *file = NULL;
	const char *postfix = CP_PATHSEP_STR "plugin.xml";
	int status = CP_OK;
	int fd = -1;
	XML_Parser parser = NULL;
	ploader_context_t *context = NULL;

	do {
		int i;

		/* Construct the file name for the plug-in descriptor */
		i = strlen(path);
		if (i == 0) {
			status = CP_ERR_IO;
			break;
		}
		if (path[i-1] == CP_PATHSEP_CHAR) {
			i--;
		}
		file = malloc((i + strlen(postfix) + 1) * sizeof(char));
		if (file == NULL) {
			status = CP_ERR_RESOURCE;
			break;
		}
		strcpy(file, path);
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
		parser = XML_ParserCreate(NULL);
		if (parser == NULL) {
			status = CP_ERR_RESOURCE;
			break;
		}
		XML_SetElementHandler(parser,
			cp_start_element_handler,
			cp_end_element_handler);
		
		/* Initialize the parsing context */
		if ((context = malloc(sizeof(ploader_context_t))) == NULL) {
			status = CP_ERR_RESOURCE;
			break;
		}
		context->plugin = NULL;
		context->imports = NULL;
		if ((context->plugin = malloc(sizeof(cp_plugin_t))) == NULL
			|| (context->imports = list_create(LISTCOUNT_T_MAX)) == NULL) {
			status = CP_ERR_RESOURCE;
			break;
		}
		context->parser = parser;
		context->file = file;
		context->state = PARSER_BEGIN;
		context->error_count = 0;
		context->resource_error_count = 0;
		memset(context->plugin, 0, sizeof(cp_plugin_t));
		XML_SetUserData(parser, context);

		/* Parse the plug-in descriptor */
		while (1) {
			int bytes_read;
			void *xml_buffer;
			
			/* Get buffer from Expat */
			if ((xml_buffer = XML_GetBuffer(parser, CP_XML_PARSER_BUFFER_SIZE))
				== NULL) {
				status = CP_ERR_RESOURCE;
				break;
			}
			
			/* Read data into buffer */
			if ((bytes_read = read(fd, xml_buffer, CP_XML_PARSER_BUFFER_SIZE))
				== -1) {
				status = CP_ERR_IO;
				break;
			}

			/* Parse the data */
			if (!(i = XML_ParseBuffer(parser, bytes_read, bytes_read == 0))) {
				cpi_errorf(_("XML parsing error in %s, line %d, column %d (%s)."),
					file,
					XML_GetErrorLineNumber(parser),
					XML_GetErrorColumnNumber(parser) + 1,
					XML_ErrorString(XML_GetErrorCode(parser)));
			}
			if (!i || context->state == PARSER_ERROR) {
				status = CP_ERR_MALFORMED;
				break;
			}
			
			if (bytes_read == 0) {
				break;
			}
		}
		if (status == CP_OK) {
			if (context->state != PARSER_END || context->error_count > 0) {
				status = CP_ERR_MALFORMED;
			}
			if (context->resource_error_count > 0) {
				status = CP_ERR_RESOURCE;
			}
		}
		if (status != CP_OK) {
			break;
		}

		/* Initialize the plug-in path */
		*(file + i) = '\0';
		context->plugin->path = file;
		file = NULL;
		
	} while (0);

	/* Report possible errors */
	switch (status) {
		case CP_ERR_MALFORMED:
			cpi_errorf(_("Encountered a malformed descriptor while loading a plug-in from %s."), path);
			break;
		case CP_ERR_IO:
			cpi_errorf(_("An I/O error occurred while loading a plug-in from %s."), path);
			break;
		case CP_ERR_RESOURCE:
			cpi_errorf(_("Insufficient resources to load a plug-in from %s."), path);
			break;
	}

	/* Release persistently allocated data on failure */
	if (status != CP_OK) {
		if (file != NULL) {
			free(file);
		}
		if (context != NULL && context->plugin != NULL) {
			cpi_free_plugin(context->plugin);
		}
	}

	/* Otherwise, set plug-in pointer */
	else {
		(*plugin) = context->plugin;
	}
	
	/* Release data allocated for parsing */
	if (parser != NULL) {
		XML_ParserFree(parser);
	}
	if (fd != -1) {
		close(fd);
	}
	if (context != NULL) {
		if (context->imports != NULL) {
			list_destroy_nodes(context->imports);
			list_destroy(context->imports);
		}
		free(context);
	}

	return status;
}

static void XMLCALL cp_start_element_handler(
	void *userData, const XML_Char *name, const XML_Char **atts) {
	static const XML_Char *req_plugin_atts[] = { "name", "id", "version", NULL };
	static const XML_Char *opt_plugin_atts[] = { "provider-name" };
	ploader_context_t *context = userData;
	int i;

	/* Process element start */
	switch (context->state) {
		case PARSER_BEGIN:
			if (!strcmp(name, "plugin")) {
				check_attributes(context, atts, req_plugin_atts,
					opt_plugin_atts);
				context->state = PARSER_PLUGIN;
				for (i = 0; atts[i] != NULL; i++) {
					if (!strcmp(atts[i], "name")) {
						strncpy(context->plugin->name, atts[i+1], CP_NAME_MAX_LENGTH);
						context->plugin->name[CP_NAME_MAX_LENGTH] = '\0';
					} else if (!strcmp(atts[i], "id")) {
						strncpy(context->plugin->identifier, atts[i+1], CP_ID_MAX_LENGTH);
						context->plugin->identifier[CP_ID_MAX_LENGTH] = '\0';
					} else if (!strcmp(atts[i], "version")) {
						strncpy(context->plugin->version, atts[i+1], CP_VERSTR_MAX_LENGTH);
						context->plugin->version[CP_VERSTR_MAX_LENGTH] = '\0';
					} else if (!strcmp(atts[i], "provider-name")) {
						strncpy(context->plugin->provider_name, atts[i+1], CP_NAME_MAX_LENGTH);
						context->plugin->provider_name[CP_NAME_MAX_LENGTH] = '\0';
					}
				}
			} else {
				unexpected_element(context, name);
			}
			break;
		case PARSER_PLUGIN:
			if (!strcmp(name, "requires")) {
				context->state = PARSER_REQUIRES;
			} else {
				unexpected_element(context, name);
			}
			break;
		case PARSER_REQUIRES:
			if (!strcmp(name, "import")) {
				cp_plugin_import_t *import = NULL;
				
				if ((import = malloc(sizeof(cp_plugin_import_t))) == NULL) {
					resource_error(context);
					break;
				}
				for (i = 0; atts[i] != NULL; i++) {
					// TODO
				}
			} else {
				unexpected_element(context, name);
			}
			break;
		case PARSER_UNKNOWN:
			context->depth++;
			break;
		default:
			unexpected_element(context, name);
			break;
	}
}

static void XMLCALL cp_end_element_handler(
	void *userData, const XML_Char *name) {
	ploader_context_t *context = userData;
	
	/* Process element end */
	switch (context->state) {
		case PARSER_PLUGIN:
			if (!strcmp(name, "plugin")) {
				context->state = PARSER_END;
			}
			break;
		case PARSER_REQUIRES:
			if (!strcmp(name, "requires")) {
				context->state = PARSER_PLUGIN;
			}
			break;
		case PARSER_UNKNOWN:
			if (--context->depth < 0) {
				context->state = context->saved_state;
			}
			break;
		default:
			descriptor_errorf(context, 0, _("unexpected closing tag for %s"),
				name);
			return;
	}
}

static void unexpected_element(ploader_context_t *context, const XML_Char *elem) {
	context->saved_state = context->state;
	context->state = PARSER_UNKNOWN;
	context->depth = 0;
	descriptor_errorf(context, 1, _("ignoring unexpected element %s and its contents"), elem);
}

static int check_attributes(ploader_context_t *context, const XML_Char **atts,
	const XML_Char **req_atts, const XML_Char **opt_atts) {
	const XML_Char **a;
	int error = 0;
	
	/* Check that required attributes have non-empty values */
	for (a = req_atts; a != NULL && *a != NULL; a++) {
		const XML_Char **av;
		
		if ((av = contains_str(atts, *a, 2)) != NULL) {
			if ((*(av + 1))[0] == '\0') {
				descriptor_errorf(context, 0,
					_("required attribute %s has empty value"),
					*a);
				error = 1;
			}
		} else {
			descriptor_errorf(context, 0, _("required attribute %s missing"),
				*a);
			error = 1;
		}
	}
	
	/* Warn if there are unknown attributes */
	for (; *atts != NULL; atts += 2) {
		if (contains_str(req_atts, *atts, 1) == NULL
			&& contains_str(opt_atts, *atts, 1) == NULL) {
			descriptor_errorf(context, 1, _("ignoring unknown attribute %s"),
				*atts);
		}
	}
	
	return !error;
}

static void descriptor_errorf(ploader_context_t *context, int warn,
	const char *error_msg, ...) {
	va_list ap;
	char message[128];
	
	va_start(ap, error_msg);
	vsnprintf(message, sizeof(message), error_msg, ap);
	va_end(ap);
	message[127] = '\0';
	cpi_errorf(_("%s descriptor data in %s, line %d, column %d (%s)."),
		(warn ? "Suspicious" : "Invalid"),
		context->file,
		XML_GetCurrentLineNumber(context->parser),
		XML_GetCurrentColumnNumber(context->parser) + 1,
		message);
	if (!warn) {
		context->error_count++;
	}
}

static void resource_error(ploader_context_t *context) {
	cpi_errorf(_("Insufficient resources to parse descriptor data in %s, line %d, column %d."),
		context->file,
		XML_GetCurrentLineNumber(context->parser),
		XML_GetCurrentColumnNumber(context->parser) + 1);
	context->resource_error_count++;
}

static const XML_Char **contains_str(const XML_Char **list,
	const XML_Char *str, int step) {
	if (list != NULL) {
		while (*list != NULL) {
			if (!strcmp(*list, str)) {
				return list;
			}
			list += step;
		}
	}
	return NULL;
}
