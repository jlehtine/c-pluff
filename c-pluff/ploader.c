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


/* ------------------------------------------------------------------------
 * Constants
 * ----------------------------------------------------------------------*/

/* Path separator characters to be used */

#ifdef __WIN32__

#define CP_PATHSEP_CHAR '\\'
#define CP_PATHSEP_STR N_("\\")

#else /*__WIN32__*/

#define CP_PATHSEP_CHAR '/'
#define CP_PATHSEP_STR N_("/")

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
	char *file;
	
	/** The plug-in being constructed */
	cp_plugin_t *plugin;
	
	/** The configuration element being constructed */
	cp_cfg_element_t *configuration;
	
	/** The current parser state */
	parser_state_t state;
	
	/** The saved parser state (used in PARSER_UNKNOWN) */
	parser_state_t saved_state;
	
	/**
	 * The current parser depth (used in PARSER_UNKNOWN and PARSER_EXTENSION)
	 */
	int depth;

	/** Size of allocated imports table */
	size_t imports_size;
	
	/** Size of allocated extension points table */
	size_t ext_points_size;
	
	/** Size of allocated extensions table */
	size_t extensions_size;
	
	/** Buffer for a value being read */
	char *value;
	
	/** Size of allocated value field */
	size_t value_size;
	
	/** The number of parsing errors that have occurred */
	int error_count;
	
	/** The number of resource errors that have occurred */
	int resource_error_count;
};


/* ------------------------------------------------------------------------
 * Function definitions
 * ----------------------------------------------------------------------*/

/* Plug-in loading */

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
	const char *error_msg, ...) {
	va_list ap;
	char message[128];
	
	va_start(ap, error_msg);
	vsnprintf(message, sizeof(message), error_msg, ap);
	va_end(ap);
	message[127] = '\0';
	cpi_errorf(warn
				? _("Suspicious descriptor data in %s, line %d, column %d (%s).")
				: _("Invalid descriptor data in %s, line %d, column %d (%s)."),
		context->file,
		XML_GetCurrentLineNumber(context->parser),
		XML_GetCurrentColumnNumber(context->parser) + 1,
		message);
	if (!warn) {
		context->error_count++;
	}
}

/**
 * Reports insufficient resources while parsing and increments the
 * resource error count.
 * 
 * @param context the parsing context
 */
static void resource_error(ploader_context_t *context) {
	if (context->resource_error_count == 0) {
		cpi_errorf(_("Insufficient resources to parse descriptor data in %s, line %d, column %d."),
			context->file,
			XML_GetCurrentLineNumber(context->parser),
			XML_GetCurrentColumnNumber(context->parser) + 1);
	}
	context->resource_error_count++;
}

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
static const XML_Char * const *contains_str(const XML_Char * const *list,
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

/**
 * Checks that an element has non-empty values for required attributes and
 * warns if there are unknown attributes. Increments the error count for
 * each missing required attribute.
 * 
 * @param context the parsing context
 * @param elem the element being checked
 * @param atts the attribute list for the element
 * @param req_atts the required attributes (NULL terminated list, or NULL)
 * @param opt_atts the optional attributes (NULL terminated list, or NULL)
 * @return whether the required attributes are present
 */
static int check_attributes(ploader_context_t *context,
	const XML_Char *elem, const XML_Char * const *atts,
	const XML_Char * const *req_atts, const XML_Char * const *opt_atts) {
	const XML_Char * const *a;
	int error = 0;
	
	/* Check that required attributes have non-empty values */
	for (a = req_atts; a != NULL && *a != NULL; a++) {
		const XML_Char * const *av;
		
		if ((av = contains_str(atts, *a, 2)) != NULL) {
			if ((*(av + 1))[0] == '\0') {
				descriptor_errorf(context, 0,
					_("required attribute \"%s\" for element \"%s\" has an empty value"),
					*a, elem);
				error = 1;
			}
		} else {
			descriptor_errorf(context, 0,
				_("required attribute \"%s\" missing for element \"%s\""),
				*a, elem);
			error = 1;
		}
	}
	
	/* Warn if there are unknown attributes */
	for (; *atts != NULL; atts += 2) {
		if (contains_str(req_atts, *atts, 1) == NULL
			&& contains_str(opt_atts, *atts, 1) == NULL) {
			descriptor_errorf(context, 1,
				_("ignoring unknown attribute \"%s\" for element \"%s\""),
				*atts, elem);
		}
	}
	
	return !error;
}

/**
 * Allocates memory using malloc. Reports a resource error if there is not
 * enough available memory.
 * 
 * @param context the parsing context
 * @param size the number of bytes to allocate
 * @return pointer to the allocated memory, or NULL if memory allocation failed
 */
static void *parser_malloc(ploader_context_t *context, size_t size) {
	void *ptr;
	
	if ((ptr = malloc(size)) == NULL) {
		resource_error(context);
	}
	return ptr;
}

/**
 * Makes a copy of the specified string. The memory is allocated using malloc.
 * Reports a resource error if there is not enough available memory.
 * 
 * @param context the parsing context
 * @param src the source string to be copied
 * @return copy of the string, or NULL if memory allocation failed
 */
static char *parser_strdup(ploader_context_t *context, const char *src) {
	char *dst;
	
	if ((dst = parser_malloc(context, sizeof(char) * (strlen(src) + 1))) != NULL) {
		strcpy(dst, src);
	}
	return dst;
}

/**
 * Concatenates the specified strings into a new string. The memory for the concatenated
 * string is allocated using malloc. Reports a resource error if there is not
 * enough available memory.
 * 
 * @param context the parsing context
 * @param ... the strings to be concatenated, terminated by NULL
 * @return the concatenated string, or NULL if memory allocation failed
 */
static char *parser_strscat(ploader_context_t *context, ...) {
	va_list ap;
	const char *str;
	char *dst;
	int len;
	
	/* Calculate the length of the concatenated string */
	va_start(ap, context);
	len = 0;
	while ((str = va_arg(ap, const char *)) != NULL) {
		len += strlen(str);
	}
	va_end(ap);
	
	/* Allocate space for the concatenated string */
	if ((dst = parser_malloc(context, sizeof(char) * (len + 1))) == NULL) {
		return NULL;
	}
	
	/* Copy the strings */
	len = 0;
	va_start(ap, context);
	while ((str = va_arg(ap, const char *)) != NULL) {
		strcpy(dst + len, str);
		len += strlen(str);
	}
	va_end(ap);
	dst[len] = '\0';
	return dst;
}

/**
 * Puts the parser to a state in which it skips an unknown element.
 * Warns error handlers about the unknown element.
 * 
 * @param context the parsing context
 * @param elem the element name
 */
static void unexpected_element(ploader_context_t *context, const XML_Char *elem) {
	context->saved_state = context->state;
	context->state = PARSER_UNKNOWN;
	context->depth = 0;
	descriptor_errorf(context, 1, _("ignoring unexpected element %s and its contents"), elem);
}

/**
 * Creates a copy of the specified attributes.
 * 
 * @param context the parser context
 * @param src the source attributes to be copied
 * @param num pointer to the location where number of attributes is stored,
 * 			or NULL for none
 * @return the duplicated attribute array
 */
static char **parser_attsdup(ploader_context_t *context, const XML_Char * const *src,
	int *num_atts) {
	char **atts, *attr_data;
	int i;
	int num;
	size_t attr_size;
	
	/* Calculate the number of attributes and the amount of space required */
	for (i = 0, num = 0, attr_size = 0; src[i] != NULL; i++) {
		num++;
		attr_size += strlen(src[i]) + 1;
	}
	assert((num & 1) == 0);
	
	/* Allocate necessary memory and copy attribute data */
	if ((atts = malloc((num + 2) * sizeof(char *))) != NULL) {
		if ((attr_data = malloc(attr_size * sizeof(char))) != NULL) {
			size_t offset;
			
			for (i = 0, offset = 0; i < num; i++) {
				strcpy(attr_data + offset, src[i]);
				atts[i] = attr_data + offset;
				offset += strlen(src[i]);
			}
			atts[i++] = NULL;
			atts[i] = NULL;
		}
	}
	
	/* If successful then return duplicates, otherwise free any allocations */
	if (atts != NULL && attr_data != NULL) {
		if (num_atts != NULL) {
			*num_atts = num / 2;
		}
		return atts;
	} else {
		if (atts != NULL) {
			free(atts);
		}
		resource_error(context);
		return NULL;
	}
}

/**
 * Allocates and parses a new configuration element.
 * 
 * @param context the parser context
 * @param name the element name
 * @param atts the element attributes
 * @param parent the parent element
 * @return the newly parsed configuration element or NULL on error
 */
static cp_cfg_element_t *parse_cfg_element(ploader_context_t *context,
	const XML_Char *name, const XML_Char * const *atts, cp_cfg_element_t *parent) {
	cp_cfg_element_t *ce;
	
	/* Allocate memory for the configuration element */
	if ((ce = malloc(sizeof(cp_cfg_element_t))) == NULL) {
		resource_error(context);
		return NULL;
	}
	
	/* Initialize the configuration element */
	memset(ce, 0, sizeof(cp_cfg_element_t));
	ce->name = parser_strdup(context, name);
	ce->atts = parser_attsdup(context, atts, &(ce->num_atts));
	ce->value = NULL;
	context->value = NULL;
	context->value_size = 0;
	ce->parent = parent;
	ce->children = NULL;	
	return ce;
}

/**
 * Processes the start of element events while parsing.
 * 
 * @param context the parsing context
 * @param name the element name
 * @param atts the element attributes
 */
static void XMLCALL start_element_handler(
	void *userData, const XML_Char *name, const XML_Char * const *atts) {
	static const XML_Char * const req_plugin_atts[] = { N_("name"), N_("id"), N_("version"), NULL };
	static const XML_Char * const opt_plugin_atts[] = { N_("provider-name"), NULL };
	static const XML_Char * const req_import_atts[] = { N_("plugin"), NULL };
	static const XML_Char * const opt_import_atts[] = { N_("version"), N_("match"), N_("optional"), NULL };
	static const XML_Char * const req_runtime_atts[] = { N_("library"), NULL };
	static const XML_Char * const opt_runtime_atts[] = { N_("start-func"), N_("stop-func"), NULL };
	static const XML_Char * const req_ext_point_atts[] = { N_("name"), N_("id"), NULL };
	static const XML_Char * const opt_ext_point_atts[] = { N_("schema"), NULL };
	static const XML_Char * const req_extension_atts[] = { N_("point"), NULL };
	static const XML_Char * const opt_extension_atts[] = { N_("id"), N_("name"), NULL };
	ploader_context_t *context = userData;
	int i;

	/* Process element start */
	switch (context->state) {
		case PARSER_BEGIN:
			if (!strcmp(name, N_("plugin"))) {
				context->state = PARSER_PLUGIN;
				if (!check_attributes(context, name, atts,
						req_plugin_atts, opt_plugin_atts)) {
					break;
				}
				for (i = 0; atts[i] != NULL; i += 2) {
					if (!strcmp(atts[i], N_("name"))) {
						context->plugin->name
							= parser_strdup(context, atts[i+1]);
					} else if (!strcmp(atts[i], N_("id"))) {
						context->plugin->identifier
							= parser_strdup(context, atts[i+1]);
					} else if (!strcmp(atts[i], N_("version"))) {
						context->plugin->version
							= parser_strdup(context, atts[i+1]);
					} else if (!strcmp(atts[i], N_("provider-name"))) {
						context->plugin->provider_name
							= parser_strdup(context, atts[i+1]);
					}
				}
			} else {
				unexpected_element(context, name);
			}
			break;
		case PARSER_PLUGIN:
			if (!strcmp(name, N_("requires"))) {
				context->state = PARSER_REQUIRES;
			} else if (!strcmp(name, N_("runtime"))) {
				if (check_attributes(context, name, atts,
						req_runtime_atts, opt_runtime_atts)) {
					for (i = 0; atts[i] != NULL; i += 2) {
						if (!strcmp(atts[i], N_("library"))) {
							context->plugin->lib_path
								= parser_strdup(context, atts[i+1]);
						} else if (!strcmp(atts[i], N_("start-func"))) {
							context->plugin->start_func_name
								= parser_strdup(context, atts[i+1]);
						} else if (!strcmp(atts[i], N_("stop-func"))) {
							context->plugin->stop_func_name
								= parser_strdup(context, atts[i+1]);
						}
					}
				}
			} else if (!strcmp(name, N_("extension-point"))) {
				if (check_attributes(context, name, atts,
						req_ext_point_atts, opt_ext_point_atts)) {
					cp_ext_point_t *ext_point;
					
					/* Allocate space for extension points, if necessary */
					if (context->plugin->num_ext_points == context->ext_points_size) {
						cp_ext_point_t *nep;
						size_t ns;
						
						if (context->ext_points_size == 0) {
							ns = 4;
						} else {
							ns = context->ext_points_size * 2;
						}
						if ((nep = realloc(context->plugin->ext_points,
								ns * sizeof(cp_ext_point_t))) == NULL) {
							resource_error(context);
							break;
						}
						context->plugin->ext_points = nep;
						context->ext_points_size = ns;
					}
					
					/* Parse extension point specification */
					ext_point = context->plugin->ext_points
						+ context->plugin->num_ext_points;
					memset(ext_point, 0, sizeof(cp_ext_point_t));
					ext_point->name = NULL;
					ext_point->local_id = NULL;
					ext_point->global_id = NULL;
					ext_point->schema_path = NULL;
					for (i = 0; atts[i] != NULL; i += 2) {
						if (!strcmp(atts[i], N_("name"))) {
							ext_point->name
								= parser_strdup(context, atts[i+1]);
						} else if (!strcmp(atts[i], N_("id"))) {
							ext_point->local_id
								= parser_strdup(context, atts[i+1]);
							ext_point->global_id
								= parser_strscat(context,
									context->plugin->identifier, N_("."), atts[i+1], NULL);
						} else if (!strcmp(atts[i], N_("schema"))) {
							ext_point->schema_path
								= parser_strdup(context, atts[i+1]);
						}
					}
					context->plugin->num_ext_points++;
					
				}
			} else if (!(strcmp(name, N_("extension")))) {
				context->state = PARSER_EXTENSION;
				context->depth = 0;
				if (check_attributes(context, name, atts,
						req_extension_atts, opt_extension_atts)) {
					cp_extension_t *extension;
				
					/* Allocate space for extensions, if necessary */
					if (context->plugin->num_extensions == context->extensions_size) {
						cp_extension_t *ne;
						size_t ns;
						
						if (context->extensions_size == 0) {
							ns = 16;
						} else {
							ns = context->extensions_size * 2;
						}
						if ((ne = realloc(context->plugin->extensions,
								ns * sizeof(cp_extension_t))) == NULL) {
							resource_error(context);
							break;
						}
						context->plugin->extensions = ne;
						context->extensions_size = ns;
					}
					
					/* Parse extension attributes */
					extension = context->plugin->extensions
						+ context->plugin->num_extensions;
					memset(extension, 0, sizeof(cp_extension_t));
					extension->name = NULL;
					extension->local_id = NULL;
					extension->global_id = NULL;
					extension->ext_point_id = NULL;
					extension->configuration = NULL;
					for (i = 0; atts[i] != NULL; i += 2) {
						if (!strcmp(atts[i], N_("point"))) {
							extension->ext_point_id
								= parser_strdup(context, atts[i+1]);
						} else if (!strcmp(atts[i], N_("id"))) {
							extension->local_id
								= parser_strdup(context, atts[i+1]);
							extension->global_id
								= parser_strscat(context,
									context->plugin->identifier, N_("."), atts[i+1], NULL);
						} else if (!strcmp(atts[i], N_("name"))) {
							extension->name
								= parser_strdup(context, atts[i+1]);
						}
					}
					context->plugin->num_extensions++;
					
					/* Initialize configuration parsing */
					context->configuration = extension->configuration
						= parse_cfg_element(context, name, atts, NULL);
				}
			} else {
				unexpected_element(context, name);
			}
			break;
		case PARSER_REQUIRES:
			if (!strcmp(name, N_("import"))) {

				if (check_attributes(context, name, atts,
						req_import_atts, opt_import_atts)) {
					cp_plugin_import_t *import = NULL;
				
					/* Allocate space for imports, if necessary */
					if (context->plugin->num_imports == context->imports_size) {
						cp_plugin_import_t *ni;
						size_t ns;
					
						if (context->imports_size == 0) {
							ns = 16;
						} else {
							ns = context->imports_size * 2;
						}
						if ((ni = realloc(context->plugin->imports,
								ns * sizeof(cp_plugin_import_t))) == NULL) {
							resource_error(context);
							break;
						}
						context->plugin->imports = ni;
						context->imports_size = ns;
					}
				
					/* Parse import specification */
					import = context->plugin->imports
						+ context->plugin->num_imports;
					memset(import, 0, sizeof(cp_plugin_import_t));
					import->plugin_id = NULL;
					import->version = NULL;
					for (i = 0; atts[i] != NULL; i += 2) {
						if (!strcmp(atts[i], N_("plugin"))) {
							import->plugin_id
								= parser_strdup(context, atts[i+1]);
						} else if (!strcmp(atts[i], N_("version"))) {
							import->version
								= parser_strdup(context, atts[i+1]);
						} else if (!strcmp(atts[i], N_("match"))) {
							if (!strcmp(atts[i+1], N_("perfect"))) {
								import->match = CP_MATCH_PERFECT;
							} else if (!strcmp(atts[i+1], N_("equivalent"))) {
								import->match = CP_MATCH_EQUIVALENT;
							} else if (!strcmp(atts[i+1], N_("compatible"))) {
								import->match = CP_MATCH_COMPATIBLE;
							} else if (!strcmp(atts[i+1], N_("greaterOrEqual"))) {
								import->match = CP_MATCH_GREATEROREQUAL;
							} else {
								descriptor_errorf(context, 0, _("unknown version matching mode %s"), atts[i+1]);
							}
						} else if (!strcmp(atts[i], N_("optional"))) {
							if (!strcmp(atts[i+1], N_("true"))
								|| !strcmp(atts[i+1], N_("1"))) {
								import->optional = 1;
							} else if (strcmp(atts[i+1], N_("false"))
								&& strcmp(atts[i+1], N_("0"))) {
								descriptor_errorf(context, 0, _("unknown boolean value %s"), atts[i+1]);
							}
						}
					}
					if (import->match != CP_MATCH_NONE
						&& (import->version == NULL || import->version[0] == '\0')) {
						descriptor_errorf(context, 0, _("unable to match unspecified or empty version"));
					}
				}
			} else {
				unexpected_element(context, name);
			}
			break;
		case PARSER_EXTENSION:
			if (context->configuration != NULL) {
				context->configuration = parse_cfg_element(context, name, atts, context->configuration);
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

/**
 * Processes the end of element events while parsing.
 * 
 * @param context the parsing context
 * @param name the element name
 */
static void XMLCALL end_element_handler(
	void *userData, const XML_Char *name) {
	ploader_context_t *context = userData;
	
	/* Process element end */
	switch (context->state) {
		case PARSER_PLUGIN:
			if (!strcmp(name, N_("plugin"))) {
				
				/* Readjust memory allocated for extension points, if necessary */
				if (context->ext_points_size != context->plugin->num_ext_points) {
					cp_ext_point_t *nep;
					
					if ((nep = realloc(context->plugin->ext_points,
							context->plugin->num_ext_points *
								sizeof(cp_ext_point_t))) != NULL
						|| context->plugin->num_ext_points == 0) {
						context->plugin->ext_points = nep;
						context->ext_points_size = context->plugin->num_ext_points;
					}
				}
				
				context->state = PARSER_END;
			}
			break;
		case PARSER_REQUIRES:
			if (!strcmp(name, N_("requires"))) {
				
				/* Readjust memory allocated for imports, if necessary */
				if (context->imports_size != context->plugin->num_imports) {
					cp_plugin_import_t *ni;
					
					if ((ni = realloc(context->plugin->imports,
							context->plugin->num_imports *
								sizeof(cp_plugin_import_t))) != NULL
						|| context->plugin->num_imports == 0) {
						context->plugin->imports = ni;
						context->imports_size = context->plugin->num_imports;
					}
				}
				
				context->state = PARSER_PLUGIN;
			}
			break;
		case PARSER_UNKNOWN:
			if (--context->depth < 0) {
				context->state = context->saved_state;
			}
			break;
		case PARSER_EXTENSION:
			if (context->configuration != NULL) {
				context->configuration->value = parser_strdup(context, context->value);
				context->configuration = context->configuration->parent;
			}			
			if (--context->depth < 0) {
				assert(!strcmp(name, N_("extension")));
				context->state = PARSER_PLUGIN;
			}
			break;
		default:
			descriptor_errorf(context, 0, _("unexpected closing tag for %s"),
				name);
			return;
	}
}

/**
 * Loads a plug-in from the specified path.
 * 
 * @param path the plug-in path
 * @param plugin where to store the pointer to the loaded plug-in
 * @return CP_OK (0) on success, an error code on failure
 */
static int load_plugin(const char *path, cp_plugin_t **plugin) {
	char *file = NULL;
	const char *postfix = CP_PATHSEP_STR N_("plugin.xml");
	int status = CP_OK;
	int fd = -1;
	XML_Parser parser = NULL;
	ploader_context_t *context = NULL;

	do {
		int path_len;

		/* Construct the file name for the plug-in descriptor */
		path_len = strlen(path);
		if (path_len == 0) {
			status = CP_ERR_IO;
			break;
		}
		if (path[path_len-1] == CP_PATHSEP_CHAR) {
			path_len--;
		}
		file = malloc((path_len + strlen(postfix) + 1) * sizeof(char));
		if (file == NULL) {
			status = CP_ERR_RESOURCE;
			break;
		}
		strcpy(file, path);
		strcpy(file + path_len, postfix);

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
			start_element_handler,
			end_element_handler);
		
		/* Initialize the parsing context */
		if ((context = malloc(sizeof(ploader_context_t))) == NULL) {
			status = CP_ERR_RESOURCE;
			break;
		}
		memset(context, 0, sizeof(ploader_context_t));
		if ((context->plugin = malloc(sizeof(cp_plugin_t))) == NULL) {
			status = CP_ERR_RESOURCE;
			break;
		}
		context->configuration = NULL;
		context->value = NULL;
		context->parser = parser;
		context->file = file;
		context->state = PARSER_BEGIN;
		memset(context->plugin, 0, sizeof(cp_plugin_t));
		context->plugin->name = NULL;
		context->plugin->identifier = NULL;
		context->plugin->version = NULL;
		context->plugin->provider_name = NULL;
		context->plugin->path = NULL;
		context->plugin->imports = NULL;
		context->plugin->lib_path = NULL;
		context->plugin->start_func_name = NULL;
		context->plugin->stop_func_name = NULL;
		context->plugin->ext_points = NULL;
		context->plugin->extensions = NULL;
		XML_SetUserData(parser, context);

		/* Parse the plug-in descriptor */
		while (1) {
			int bytes_read;
			void *xml_buffer;
			int i;
			
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
		*(file + path_len) = '\0';
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
			file = NULL;
		}
		if (context != NULL && context->plugin != NULL) {
			cpi_free_plugin(context->plugin);
			context->plugin = NULL;
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
		if (context->value != NULL) {
			free(context->value);
		}
		free(context);
		context = NULL;
	}

	return status;
}

int CP_API cp_load_plugin(const char *path, char **id) {
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
	if (status == CP_OK && id != NULL) {
		*id = plugin->identifier;
	}
	
	/* Release any allocated data on failure */
	else {
		if (plugin != NULL) {
			cpi_free_plugin(plugin);
		}
	}
	
	return status;
}

int CP_API cp_rescan_plugins(const char *dir, int flags) {
	assert(dir != NULL);
	/* TODO */
	return CP_ERR_UNSPECIFIED;
}

