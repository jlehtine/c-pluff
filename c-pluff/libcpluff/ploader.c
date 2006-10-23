/*-------------------------------------------------------------------------
 * C-Pluff, a plug-in framework for C
 * Copyright 2006 Johannes Lehtinen
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
#include <dirent.h>
#include <expat.h>
#include <errno.h>
#include "cpluff.h"
#include "defines.h"
#include "util.h"
#include "context.h"
#include "pcontrol.h"


/* ------------------------------------------------------------------------
 * Constants
 * ----------------------------------------------------------------------*/

/// XML parser buffer size (in bytes) 
#define CP_XML_PARSER_BUFFER_SIZE 4096

/// Initial configuration element value size 
#define CP_CFG_ELEMENT_VALUE_INITSIZE 64

/// Plugin descriptor name 
#define CP_PLUGIN_DESCRIPTOR "plugin.xml"


/* ------------------------------------------------------------------------
 * Internal data types
 * ----------------------------------------------------------------------*/

typedef struct ploader_context_t ploader_context_t;

/// Parser states 
typedef enum parser_state_t {
	PARSER_BEGIN,
	PARSER_PLUGIN,
	PARSER_REQUIRES,
	PARSER_EXTENSION,
	PARSER_END,
	PARSER_UNKNOWN,
	PARSER_ERROR
} parser_state_t;

/// Plug-in loader context 
struct ploader_context_t {

	/// The XML parser being used 
	XML_Parser parser;
	
	/// The file being parsed 
	char *file;
	
	/// The plug-in being constructed 
	cp_plugin_t *plugin;
	
	/// The configuration element being constructed 
	cp_cfg_element_t *configuration;
	
	/// The current parser state 
	parser_state_t state;
	
	/// The saved parser state (used in PARSER_UNKNOWN) 
	parser_state_t saved_state;
	
	/**
	 * The current parser depth (used in PARSER_UNKNOWN and PARSER_EXTENSION)
	 */
	unsigned int depth;
	
	/// The number of skipped configuration elements 
	unsigned int skippedCEs;

	/// Size of allocated imports table 
	size_t imports_size;
	
	/// Size of allocated extension points table 
	size_t ext_points_size;
	
	/// Size of allocated extensions table 
	size_t extensions_size;
	
	/// Buffer for a value being read 
	char *value;
	
	/// Size of allocated value field 
	size_t value_size;
	
	/// Current length of value string 
	size_t value_length;
	
	/// The number of parsing errors that have occurred 
	unsigned int error_count;
	
	/// The number of resource errors that have occurred 
	unsigned int resource_error_count;
};


/* ------------------------------------------------------------------------
 * Function definitions
 * ----------------------------------------------------------------------*/

// Plug-in loading 

/**
 * Reports a descriptor error. Does not set the parser to error state but
 * increments the error count, unless this is merely a warning.
 * 
 * @param context the parsing context
 * @param warn whether this is only a warning
 * @param error_msg the error message
 * @param ... parameters for the error message
 */
static void descriptor_errorf(ploader_context_t *plcontext, int warn,
	const char *error_msg, ...) {
	va_list ap;
	char message[128];
	
	va_start(ap, error_msg);
	vsnprintf(message, sizeof(message), error_msg, ap);
	va_end(ap);
	message[127] = '\0';
	cpi_errorf(plcontext->plugin->context,
		warn
			? _("Suspicious descriptor data in %s, line %d, column %d (%s).")
			: _("Invalid descriptor data in %s, line %d, column %d (%s)."),
		plcontext->file,
		XML_GetCurrentLineNumber(plcontext->parser),
		XML_GetCurrentColumnNumber(plcontext->parser) + 1,
		message);
	if (!warn) {
		plcontext->error_count++;
	}
}

/**
 * Reports insufficient system resources while parsing and increments the
 * resource error count.
 * 
 * @param context the parsing context
 */
static void resource_error(ploader_context_t *plcontext) {
	if (plcontext->resource_error_count == 0) {
		cpi_errorf(plcontext->plugin->context,
			_("Insufficient system resources to parse descriptor data in %s, line %d, column %d."),
			plcontext->file,
			XML_GetCurrentLineNumber(plcontext->parser),
			XML_GetCurrentColumnNumber(plcontext->parser) + 1);
	}
	plcontext->resource_error_count++;
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
static int check_attributes(ploader_context_t *plcontext,
	const XML_Char *elem, const XML_Char * const *atts,
	const XML_Char * const *req_atts, const XML_Char * const *opt_atts) {
	const XML_Char * const *a;
	int error = 0;
	
	// Check that required attributes have non-empty values 
	for (a = req_atts; a != NULL && *a != NULL; a++) {
		const XML_Char * const *av;
		
		if ((av = contains_str(atts, *a, 2)) != NULL) {
			if ((*(av + 1))[0] == '\0') {
				descriptor_errorf(plcontext, 0,
					_("required attribute %s for element %s has an empty value"),
					*a, elem);
				error = 1;
			}
		} else {
			descriptor_errorf(plcontext, 0,
				_("required attribute %s missing for element %s"),
				*a, elem);
			error = 1;
		}
	}
	
	// Warn if there are unknown attributes 
	for (; *atts != NULL; atts += 2) {
		if (contains_str(req_atts, *atts, 1) == NULL
			&& contains_str(opt_atts, *atts, 1) == NULL) {
			descriptor_errorf(plcontext, 1,
				_("ignoring unknown attribute %s for element %s"),
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
#ifdef HAVE_DMALLOC_H
static void *parser_malloc_dm(ploader_context_t *plcontext, size_t size, int line) {
#else
static void *parser_malloc(ploader_context_t *plcontext, size_t size) {
#endif
	void *ptr;

#ifdef HAVE_DMALLOC_H
	ptr = dmalloc_malloc(__FILE__, line, size, DMALLOC_FUNC_MALLOC, 0, 0);
#else
	ptr = malloc(size);
#endif	
	if (ptr == NULL) {
		resource_error(plcontext);
	}
	return ptr;
}

#ifdef HAVE_DMALLOC_H
#define parser_malloc(a, b) parser_malloc_dm((a), (b), __LINE__)
#endif

/**
 * Makes a copy of the specified string. The memory is allocated using malloc.
 * Reports a resource error if there is not enough available memory.
 * 
 * @param context the parsing context
 * @param src the source string to be copied
 * @return copy of the string, or NULL if memory allocation failed
 */
#ifdef HAVE_DMALLOC_H
static char *parser_strdup_dm(ploader_context_t *plcontext, const char *src, int line) {
#else
static char *parser_strdup(ploader_context_t *plcontext, const char *src) {
#endif
	char *dst;

#ifdef HAVE_DMALLOC_H
	dst = parser_malloc_dm(plcontext, sizeof(char) * (strlen(src) + 1), line);
#else
	dst = parser_malloc(plcontext, sizeof(char) * (strlen(src) + 1));
#endif
	if (dst != NULL) {
		strcpy(dst, src);
	}
	return dst;
}

#ifdef HAVE_DMALLOC_H
#define parser_strdup(a, b) parser_strdup_dm((a), (b), __LINE__)
#endif

/**
 * Concatenates the specified strings into a new string. The memory for the concatenated
 * string is allocated using malloc. Reports a resource error if there is not
 * enough available memory.
 * 
 * @param context the parsing context
 * @param ... the strings to be concatenated, terminated by NULL
 * @return the concatenated string, or NULL if memory allocation failed
 */
static char *parser_strscat(ploader_context_t *plcontext, ...) {
	va_list ap;
	const char *str;
	char *dst;
	size_t len;
	
	// Calculate the length of the concatenated string 
	va_start(ap, plcontext);
	len = 0;
	while ((str = va_arg(ap, const char *)) != NULL) {
		len += strlen(str);
	}
	va_end(ap);
	
	// Allocate space for the concatenated string 
	if ((dst = parser_malloc(plcontext, sizeof(char) * (len + 1))) == NULL) {
		return NULL;
	}
	
	// Copy the strings 
	len = 0;
	va_start(ap, plcontext);
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
static void unexpected_element(ploader_context_t *plcontext, const XML_Char *elem) {
	plcontext->saved_state = plcontext->state;
	plcontext->state = PARSER_UNKNOWN;
	plcontext->depth = 0;
	descriptor_errorf(plcontext, 1, _("ignoring unexpected element %s and its contents"), elem);
}

/**
 * Creates a copy of the specified attributes. Reports failed memory
 * allocation.
 * 
 * @param context the parser context
 * @param src the source attributes to be copied
 * @param num pointer to the location where number of attributes is stored,
 * 			or NULL for none
 * @return the duplicated attribute array, or NULL if empty or failed
 */
static char **parser_attsdup(ploader_context_t *plcontext, const XML_Char * const *src,
	unsigned int *num_atts) {
	char **atts = NULL, *attr_data = NULL;
	unsigned int i;
	unsigned int num;
	size_t attr_size;
	
	// Calculate the number of attributes and the amount of space required 
	for (num = 0, attr_size = 0; src[num] != NULL; num++) {
		attr_size += strlen(src[num]) + 1;
	}
	assert((num & 1) == 0);
	
	// Allocate necessary memory and copy attribute data 
	if (num > 0) {
		if ((atts = parser_malloc(plcontext, num * sizeof(char *))) != NULL) {
			if ((attr_data = parser_malloc(plcontext, attr_size * sizeof(char))) != NULL) {
				size_t offset;
			
				for (i = 0, offset = 0; i < num; i++) {
					strcpy(attr_data + offset, src[i]);
					atts[i] = attr_data + offset;
					offset += strlen(src[i]) + 1;
				}
			}
		}
	}
	
	// If successful then return duplicates, otherwise free any allocations 
	if (num == 0 || (atts != NULL && attr_data != NULL)) {
		if (num_atts != NULL) {
			*num_atts = num / 2;
		}
		return atts;
	} else {
		free(attr_data);
		free(atts);
		return NULL;
	}
}

/**
 * Initializes a configuration element. Reports an error if memory allocation fails.
 * 
 * @param context the parser context
 * @param ce the configuration element to be initialized
 * @param name the element name
 * @param atts the element attributes
 * @param parent the parent element
 */
static void init_cfg_element(ploader_context_t *plcontext, cp_cfg_element_t *ce,
	const XML_Char *name, const XML_Char * const *atts, cp_cfg_element_t *parent) {
	
	// Initialize the configuration element 
	memset(ce, 0, sizeof(cp_cfg_element_t));
	ce->name = parser_strdup(plcontext, name);
	ce->atts = parser_attsdup(plcontext, atts, &(ce->num_atts));
	ce->value = NULL;
	plcontext->value = NULL;
	plcontext->value_size = 0;
	plcontext->value_length = 0;
	ce->parent = parent;
	ce->children = NULL;	
}

// TODO
static void XMLCALL character_data_handler(
	void *userData, const XML_Char *str, int len) {
	ploader_context_t *plcontext = userData;
	
	// Ignore leading whitespace 
	if (plcontext->value == NULL) {
		int i;
		
		for (i = 0; i < len; i++) {
			if (str[i] != ' ' && str[i] != '\n' && str[i] != '\r' && str[i] != '\t') {
				break;
			}
		}
		str += i;
		len -= i;
		if (len == 0) {
			return;
		}
	}
	
	// Allocate more memory for the character data if needed 
	if (plcontext->value_length + len >= plcontext->value_size) {
		size_t ns;
		char *nv;

		ns = plcontext->value_size;
		while (plcontext->value_length + len >= ns) {		
			if (ns == 0) {
				ns = CP_CFG_ELEMENT_VALUE_INITSIZE;
			} else {
				ns = 2 * ns;
			}
		}
		if ((nv = realloc(plcontext->value, ns * sizeof(char))) != NULL) {
			plcontext->value = nv;
			plcontext->value_size = ns;
		} else {
			resource_error(plcontext);
			return;
		}
	}
	
	// Copy character data 
	strncpy(plcontext->value + plcontext->value_length, str, len * sizeof(char));
	plcontext->value_length += len;
}

/**
 * Processes the start of element events while parsing.
 * 
 * @param context the parsing context
 * @param name the element name
 * @param atts the element attributes
 */
static void XMLCALL start_element_handler(
	void *userData, const XML_Char *name, const XML_Char **atts) {
	static const XML_Char * const req_plugin_atts[] = { "name", "id", "version", NULL };
	static const XML_Char * const opt_plugin_atts[] = { "provider-name", NULL };
	static const XML_Char * const req_import_atts[] = { "plugin", NULL };
	static const XML_Char * const opt_import_atts[] = { "version", "match", "optional", NULL };
	static const XML_Char * const req_runtime_atts[] = { "library", NULL };
	static const XML_Char * const opt_runtime_atts[] = { "start-func", "stop-func", NULL };
	static const XML_Char * const req_ext_point_atts[] = { "name", "id", NULL };
	static const XML_Char * const opt_ext_point_atts[] = { "schema", NULL };
	static const XML_Char * const req_extension_atts[] = { "point", NULL };
	static const XML_Char * const opt_extension_atts[] = { "id", "name", NULL };
	ploader_context_t *plcontext = userData;
	unsigned int i;

	// Process element start 
	switch (plcontext->state) {

		case PARSER_BEGIN:
			if (!strcmp(name, "plugin")) {
				plcontext->state = PARSER_PLUGIN;
				if (!check_attributes(plcontext, name, atts,
						req_plugin_atts, opt_plugin_atts)) {
					break;
				}
				for (i = 0; atts[i] != NULL; i += 2) {
					if (!strcmp(atts[i], "name")) {
						plcontext->plugin->name
							= parser_strdup(plcontext, atts[i+1]);
					} else if (!strcmp(atts[i], "id")) {
						plcontext->plugin->identifier
							= parser_strdup(plcontext, atts[i+1]);
					} else if (!strcmp(atts[i], "version")) {
						plcontext->plugin->version
							= parser_strdup(plcontext, atts[i+1]);
					} else if (!strcmp(atts[i], "provider-name")) {
						plcontext->plugin->provider_name
							= parser_strdup(plcontext, atts[i+1]);
					}
				}
			} else {
				unexpected_element(plcontext, name);
			}
			break;

		case PARSER_PLUGIN:
			if (!strcmp(name, "requires")) {
				plcontext->state = PARSER_REQUIRES;
			} else if (!strcmp(name, "runtime")) {
				if (check_attributes(plcontext, name, atts,
						req_runtime_atts, opt_runtime_atts)) {
					for (i = 0; atts[i] != NULL; i += 2) {
						if (!strcmp(atts[i], "library")) {
							plcontext->plugin->lib_path
								= parser_strdup(plcontext, atts[i+1]);
						} else if (!strcmp(atts[i], "start-func")) {
							plcontext->plugin->start_func_name
								= parser_strdup(plcontext, atts[i+1]);
						} else if (!strcmp(atts[i], "stop-func")) {
							plcontext->plugin->stop_func_name
								= parser_strdup(plcontext, atts[i+1]);
						}
					}
				}
			} else if (!strcmp(name, "extension-point")) {
				if (check_attributes(plcontext, name, atts,
						req_ext_point_atts, opt_ext_point_atts)) {
					cp_ext_point_t *ext_point;
					
					// Allocate space for extension points, if necessary 
					if (plcontext->plugin->num_ext_points == plcontext->ext_points_size) {
						cp_ext_point_t *nep;
						size_t ns;
						
						if (plcontext->ext_points_size == 0) {
							ns = 4;
						} else {
							ns = plcontext->ext_points_size * 2;
						}
						if ((nep = realloc(plcontext->plugin->ext_points,
								ns * sizeof(cp_ext_point_t))) == NULL) {
							resource_error(plcontext);
							break;
						}
						plcontext->plugin->ext_points = nep;
						plcontext->ext_points_size = ns;
					}
					
					// Parse extension point specification 
					ext_point = plcontext->plugin->ext_points
						+ plcontext->plugin->num_ext_points;
					memset(ext_point, 0, sizeof(cp_ext_point_t));
					ext_point->name = NULL;
					ext_point->local_id = NULL;
					ext_point->global_id = NULL;
					ext_point->schema_path = NULL;
					for (i = 0; atts[i] != NULL; i += 2) {
						if (!strcmp(atts[i], "name")) {
							ext_point->name
								= parser_strdup(plcontext, atts[i+1]);
						} else if (!strcmp(atts[i], "id")) {
							ext_point->local_id
								= parser_strdup(plcontext, atts[i+1]);
							ext_point->global_id
								= parser_strscat(plcontext,
									plcontext->plugin->identifier, ".", atts[i+1], NULL);
						} else if (!strcmp(atts[i], "schema")) {
							ext_point->schema_path
								= parser_strdup(plcontext, atts[i+1]);
						}
					}
					plcontext->plugin->num_ext_points++;
					
				}
			} else if (!(strcmp(name, "extension"))) {
				plcontext->state = PARSER_EXTENSION;
				plcontext->depth = 0;
				if (check_attributes(plcontext, name, atts,
						req_extension_atts, opt_extension_atts)) {
					cp_extension_t *extension;
				
					// Allocate space for extensions, if necessary 
					if (plcontext->plugin->num_extensions == plcontext->extensions_size) {
						cp_extension_t *ne;
						size_t ns;
						
						if (plcontext->extensions_size == 0) {
							ns = 16;
						} else {
							ns = plcontext->extensions_size * 2;
						}
						if ((ne = realloc(plcontext->plugin->extensions,
								ns * sizeof(cp_extension_t))) == NULL) {
							resource_error(plcontext);
							break;
						}
						plcontext->plugin->extensions = ne;
						plcontext->extensions_size = ns;
					}
					
					// Parse extension attributes 
					extension = plcontext->plugin->extensions
						+ plcontext->plugin->num_extensions;
					memset(extension, 0, sizeof(cp_extension_t));
					extension->name = NULL;
					extension->local_id = NULL;
					extension->global_id = NULL;
					extension->ext_point_id = NULL;
					extension->configuration = NULL;
					for (i = 0; atts[i] != NULL; i += 2) {
						if (!strcmp(atts[i], "point")) {
							extension->ext_point_id
								= parser_strdup(plcontext, atts[i+1]);
						} else if (!strcmp(atts[i], "id")) {
							extension->local_id
								= parser_strdup(plcontext, atts[i+1]);
							extension->global_id
								= parser_strscat(plcontext,
									plcontext->plugin->identifier, ".", atts[i+1], NULL);
						} else if (!strcmp(atts[i], "name")) {
							extension->name
								= parser_strdup(plcontext, atts[i+1]);
						}
					}
					plcontext->plugin->num_extensions++;
					
					// Initialize configuration parsing 
					if ((extension->configuration = plcontext->configuration
						= parser_malloc(plcontext, sizeof(cp_cfg_element_t))) != NULL) {
						init_cfg_element(plcontext, plcontext->configuration, name, atts, NULL);
					}
					XML_SetCharacterDataHandler(plcontext->parser, character_data_handler);
				}
			} else {
				unexpected_element(plcontext, name);
			}
			break;

		case PARSER_REQUIRES:
			if (!strcmp(name, "import")) {

				if (check_attributes(plcontext, name, atts,
						req_import_atts, opt_import_atts)) {
					cp_plugin_import_t *import = NULL;
				
					// Allocate space for imports, if necessary 
					if (plcontext->plugin->num_imports == plcontext->imports_size) {
						cp_plugin_import_t *ni;
						size_t ns;
					
						if (plcontext->imports_size == 0) {
							ns = 16;
						} else {
							ns = plcontext->imports_size * 2;
						}
						if ((ni = realloc(plcontext->plugin->imports,
								ns * sizeof(cp_plugin_import_t))) == NULL) {
							resource_error(plcontext);
							break;
						}
						plcontext->plugin->imports = ni;
						plcontext->imports_size = ns;
					}
				
					// Parse import specification 
					import = plcontext->plugin->imports
						+ plcontext->plugin->num_imports;
					memset(import, 0, sizeof(cp_plugin_import_t));
					import->plugin_id = NULL;
					import->version = NULL;
					for (i = 0; atts[i] != NULL; i += 2) {
						if (!strcmp(atts[i], "plugin")) {
							import->plugin_id
								= parser_strdup(plcontext, atts[i+1]);
						} else if (!strcmp(atts[i], "version")) {
							if (!cpi_version_isvalid(atts[i+1])) {
								descriptor_errorf(plcontext, 0, _("invalid version string: %s"), atts[i+1]);
							}
							import->version
								= parser_strdup(plcontext, atts[i+1]);
						} else if (!strcmp(atts[i], "match")) {
							if (!strcmp(atts[i+1], "perfect")) {
								import->match = CP_MATCH_PERFECT;
							} else if (!strcmp(atts[i+1], "equivalent")) {
								import->match = CP_MATCH_EQUIVALENT;
							} else if (!strcmp(atts[i+1], "compatible")) {
								import->match = CP_MATCH_COMPATIBLE;
							} else if (!strcmp(atts[i+1], "greaterOrEqual")) {
								import->match = CP_MATCH_GREATEROREQUAL;
							} else {
								descriptor_errorf(plcontext, 0, _("unknown version matching mode: %s"), atts[i+1]);
							}
						} else if (!strcmp(atts[i], "optional")) {
							if (!strcmp(atts[i+1], "true")
								|| !strcmp(atts[i+1], "1")) {
								import->optional = 1;
							} else if (strcmp(atts[i+1], "false")
								&& strcmp(atts[i+1], "0")) {
								descriptor_errorf(plcontext, 0, _("unknown boolean value: %s"), atts[i+1]);
							}
						}
					}
					if (import->match != CP_MATCH_NONE
						&& (import->version == NULL || import->version[0] == '\0')) {
						descriptor_errorf(plcontext, 0, _("unable to match unspecified or empty version"));
					}
				}
			} else {
				unexpected_element(plcontext, name);
			}
			break;

		case PARSER_EXTENSION:
			plcontext->depth++;
			if (plcontext->configuration != NULL && plcontext->skippedCEs == 0) {
				cp_cfg_element_t *ce;
				
				// Allocate more space for children, if necessary 
				if (plcontext->configuration->num_children == plcontext->configuration->index) {
					cp_cfg_element_t *nce;
					size_t ns;
						
					if (plcontext->configuration->index == 0) {
						ns = 16;
					} else {
						ns = plcontext->configuration->index * 2;
					}
					if ((nce = realloc(plcontext->configuration->children,
							ns * sizeof(cp_cfg_element_t))) == NULL) {
						plcontext->skippedCEs++;
						resource_error(plcontext);
						break;
					}
					plcontext->configuration->children = nce;
					plcontext->configuration->index = ns;
				}
				
				// Save possible value 
				if (plcontext->value != NULL) {
					plcontext->value[plcontext->value_length] = '\0';
					plcontext->configuration->value = plcontext->value;
				}
				
				ce = plcontext->configuration->children + plcontext->configuration->num_children;
				init_cfg_element(plcontext, ce, name, atts, plcontext->configuration);
				plcontext->configuration->num_children++;
				plcontext->configuration = ce;
			}
			break;
			
		case PARSER_UNKNOWN:
			plcontext->depth++;
			break;
		default:
			unexpected_element(plcontext, name);
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
	ploader_context_t *plcontext = userData;
	
	// Process element end 
	switch (plcontext->state) {

		case PARSER_PLUGIN:
			if (!strcmp(name, "plugin")) {
				
				// Readjust memory allocated for extension points, if necessary 
				if (plcontext->ext_points_size != plcontext->plugin->num_ext_points) {
					cp_ext_point_t *nep;
					
					if ((nep = realloc(plcontext->plugin->ext_points,
							plcontext->plugin->num_ext_points *
								sizeof(cp_ext_point_t))) != NULL
						|| plcontext->plugin->num_ext_points == 0) {
						plcontext->plugin->ext_points = nep;
						plcontext->ext_points_size = plcontext->plugin->num_ext_points;
					}
				}
				
				// Readjust memory allocated for extensions, if necessary 
				if (plcontext->extensions_size != plcontext->plugin->num_extensions) {
					cp_extension_t *ne;
					
					if ((ne = realloc(plcontext->plugin->extensions,
							plcontext->plugin->num_extensions *
								sizeof(cp_extension_t))) != NULL
						|| plcontext->plugin->num_extensions == 0) {
						plcontext->plugin->extensions = ne;
						plcontext->extensions_size = plcontext->plugin->num_extensions;
					}					
				}
				
				plcontext->state = PARSER_END;
			}
			break;

		case PARSER_REQUIRES:
			if (!strcmp(name, "requires")) {
				
				// Readjust memory allocated for imports, if necessary 
				if (plcontext->imports_size != plcontext->plugin->num_imports) {
					cp_plugin_import_t *ni;
					
					if ((ni = realloc(plcontext->plugin->imports,
							plcontext->plugin->num_imports *
								sizeof(cp_plugin_import_t))) != NULL
						|| plcontext->plugin->num_imports == 0) {
						plcontext->plugin->imports = ni;
						plcontext->imports_size = plcontext->plugin->num_imports;
					}
				}
				
				plcontext->state = PARSER_PLUGIN;
			}
			break;

		case PARSER_UNKNOWN:
			if (plcontext->depth-- == 0) {
				plcontext->state = plcontext->saved_state;
			}
			break;

		case PARSER_EXTENSION:
			if (plcontext->skippedCEs > 0) {
				plcontext->skippedCEs--;
			} else if (plcontext->configuration != NULL) {
				
				// Readjust memory allocated for children, if necessary 
				if (plcontext->configuration->index != plcontext->configuration->num_children) {
					cp_cfg_element_t *nce;
					
					if ((nce = realloc(plcontext->configuration->children,
							plcontext->configuration->num_children *
								sizeof(cp_cfg_element_t))) != NULL
						|| plcontext->configuration->num_children == 0) {
						plcontext->configuration->children = nce;
					}
				}
				
				if (plcontext->configuration->parent != NULL) {
					plcontext->configuration->index = plcontext->configuration->parent->num_children - 1;
				} else {
					plcontext->configuration->index = 0;
				}
				if (plcontext->value != NULL) {
					char *v = plcontext->value;
					int i;
					
					// Ignore trailing whitespace 
					for (i = plcontext->value_length - 1; i >= 0; i--) {
						if (v[i] != ' ' && v[i] != '\n' && v[i] != '\r' && v[i] != '\t') {
							break;
						}
					}
					if (i  < 0) {
						free(plcontext->value);
						plcontext->value = NULL;
						plcontext->value_length = 0;
						plcontext->value_size = 0;
					} else {
						plcontext->value_length = i + 1;
					}
				}
				if (plcontext->value != NULL) {
					
					// Readjust memory allocated for value, if necessary 
					if (plcontext->value_size > plcontext->value_length + 1) {
						char *nv;
						
						if ((nv = realloc(plcontext->value, (plcontext->value_length + 1) * sizeof(char))) != NULL) {
							plcontext->value = nv;
						}
					}
					
					plcontext->value[plcontext->value_length] = '\0';
					plcontext->configuration->value = plcontext->value;
					plcontext->value = NULL;
					plcontext->value_size = 0;
					plcontext->value_length = 0;
				}
				plcontext->configuration = plcontext->configuration->parent;
				
				// Restore possible value 
				if (plcontext->configuration != NULL
					&& plcontext->configuration->value != NULL) {
					plcontext->value = plcontext->configuration->value;
					plcontext->value_length = strlen(plcontext->value);
					plcontext->value_size = CP_CFG_ELEMENT_VALUE_INITSIZE;
					while (plcontext->value_size < plcontext->value_length + 1) {
						plcontext->value_size *= 2;
					}
				}
				
			}			
			if (plcontext->depth-- == 0) {
				assert(!strcmp(name, "extension"));
				plcontext->state = PARSER_PLUGIN;
				XML_SetCharacterDataHandler(plcontext->parser, NULL);
			}
			break;
			
		default:
			descriptor_errorf(plcontext, 0, _("unexpected closing tag for %s"),
				name);
			return;
	}
}

/**
 * Loads a plug-in from the specified path.
 * 
 * @param context the loading plug-in context
 * @param path the plug-in path
 * @param plugin where to store the pointer to the loaded plug-in
 * @return CP_OK (0) on success, an error code on failure
 */
static int load_plugin(cp_context_t *context, const char *path, cp_plugin_t **plugin) {
	char *file = NULL;
	int status = CP_OK;
	FILE *fh = NULL;
	XML_Parser parser = NULL;
	ploader_context_t *plcontext = NULL;

	assert(context != NULL);
	assert(path != NULL);
	do {
		int path_len;

		// Construct the file name for the plug-in descriptor 
		path_len = strlen(path);
		if (path_len == 0) {
			status = CP_ERR_IO;
			break;
		}
		if (path[path_len - 1] == CP_FNAMESEP_CHAR) {
			path_len--;
		}
		file = malloc((path_len + strlen(CP_PLUGIN_DESCRIPTOR) + 2) * sizeof(char));
		if (file == NULL) {
			status = CP_ERR_RESOURCE;
			break;
		}
		strcpy(file, path);
		file[path_len] = CP_FNAMESEP_CHAR;
		strcpy(file + path_len + 1, CP_PLUGIN_DESCRIPTOR);

		// Open the file 
		if ((fh = fopen(file, "rb")) == NULL) {
			status = CP_ERR_IO;
			break;
		}

		// Initialize the XML parsing 
		parser = XML_ParserCreate(NULL);
		if (parser == NULL) {
			status = CP_ERR_RESOURCE;
			break;
		}
		XML_SetElementHandler(parser,
			start_element_handler,
			end_element_handler);
		
		// Initialize the parsing context 
		if ((plcontext = malloc(sizeof(ploader_context_t))) == NULL) {
			status = CP_ERR_RESOURCE;
			break;
		}
		memset(plcontext, 0, sizeof(ploader_context_t));
		if ((plcontext->plugin = malloc(sizeof(cp_plugin_t))) == NULL) {
			status = CP_ERR_RESOURCE;
			break;
		}
		plcontext->configuration = NULL;
		plcontext->value = NULL;
		plcontext->parser = parser;
		plcontext->file = file;
		plcontext->state = PARSER_BEGIN;
		memset(plcontext->plugin, 0, sizeof(cp_plugin_t));
		plcontext->plugin->context = context;
		plcontext->plugin->name = NULL;
		plcontext->plugin->identifier = NULL;
		plcontext->plugin->version = NULL;
		plcontext->plugin->provider_name = NULL;
		plcontext->plugin->path = NULL;
		plcontext->plugin->imports = NULL;
		plcontext->plugin->lib_path = NULL;
		plcontext->plugin->start_func_name = NULL;
		plcontext->plugin->stop_func_name = NULL;
		plcontext->plugin->ext_points = NULL;
		plcontext->plugin->extensions = NULL;
		XML_SetUserData(parser, plcontext);

		// Parse the plug-in descriptor 
		while (1) {
			int bytes_read;
			void *xml_buffer;
			int i;
			
			// Get buffer from Expat 
			if ((xml_buffer = XML_GetBuffer(parser, CP_XML_PARSER_BUFFER_SIZE))
				== NULL) {
				status = CP_ERR_RESOURCE;
				break;
			}
			
			// Read data into buffer 
			bytes_read = fread(xml_buffer, 1, CP_XML_PARSER_BUFFER_SIZE, fh);
			if (ferror(fh)) {
				status = CP_ERR_IO;
				break;
			}

			// Parse the data 
			if (!(i = XML_ParseBuffer(parser, bytes_read, bytes_read == 0))) {
				cpi_errorf(context,
					_("XML parsing error in %s, line %d, column %d (%s)."),
					file,
					XML_GetErrorLineNumber(parser),
					XML_GetErrorColumnNumber(parser) + 1,
					XML_ErrorString(XML_GetErrorCode(parser)));
			}
			if (!i || plcontext->state == PARSER_ERROR) {
				status = CP_ERR_MALFORMED;
				break;
			}
			
			if (bytes_read == 0) {
				break;
			}
		}
		if (status == CP_OK) {
			if (plcontext->state != PARSER_END || plcontext->error_count > 0) {
				status = CP_ERR_MALFORMED;
			}
			if (plcontext->resource_error_count > 0) {
				status = CP_ERR_RESOURCE;
			}
		}
		if (status != CP_OK) {
			break;
		}

		// Initialize the plug-in path 
		*(file + path_len) = '\0';
		plcontext->plugin->path = file;
		file = NULL;
		
	} while (0);

	// Report possible errors 
	switch (status) {
		case CP_ERR_MALFORMED:
			cpi_errorf(context,
				_("Encountered a malformed descriptor while loading a plug-in from %s."), path);
			break;
		case CP_ERR_IO:
			cpi_errorf(context,
				_("An I/O error occurred while loading a plug-in from %s."), path);
			break;
		case CP_ERR_RESOURCE:
			cpi_errorf(context,
				_("Insufficient system resources to load a plug-in from %s."), path);
			break;
	}

	// Release persistently allocated data on failure 
	if (status != CP_OK) {
		if (file != NULL) {
			free(file);
			file = NULL;
		}
		if (plcontext != NULL && plcontext->plugin != NULL) {
			cpi_free_plugin(plcontext->plugin);
			plcontext->plugin = NULL;
		}
	}

	// Otherwise, set plug-in pointer 
	else {
		(*plugin) = plcontext->plugin;
	}
	
	// Release data allocated for parsing 
	if (parser != NULL) {
		XML_ParserFree(parser);
	}
	if (fh != NULL) {
		fclose(fh);
	}
	if (plcontext != NULL) {
		if (plcontext->value != NULL) {
			free(plcontext->value);
		}
		free(plcontext);
		plcontext = NULL;
	}

	return status;
}

cp_plugin_t * CP_API cp_load_plugin(cp_context_t *context, const char *path, int *error) {
	cp_plugin_t *plugin = NULL;
	int status = CP_OK;

	assert(context != NULL);
	assert(path != NULL);
	do {

		// Load the plug-in 
		status = load_plugin(context, path, &plugin);
		if (status != CP_OK) {
			break;
		}

		// Register the plug-in 
		status = cpi_install_plugin(context, plugin, 1);
		if (status != CP_OK) {
			break;
		}
	
	} while (0);
	
	// Release any allocated data on failure 
	if (status != CP_OK) {
		if (plugin != NULL) {
			cpi_free_plugin(plugin);
			plugin = NULL;
		}
	}
	
	// Return the error code if requested 
	if (error != NULL) {
		*error = status;
	}
	
	return plugin;
}

int CP_API cp_load_plugins(cp_context_t *context, int flags) {
	hash_t *avail_plugins = NULL;
	list_t *started_plugins = NULL;
	cp_plugin_t **plugins = NULL;
	char *pdir_path = NULL;
	int pdir_path_size = 0;
	int plugins_stopped = 0;
	int status = CP_OK;
	
	cpi_lock_context(context);
	do {
		lnode_t *lnode;
		hscan_t hscan;
		hnode_t *hnode;
	
		// Create a hash for available plug-ins 
		if ((avail_plugins = hash_create(HASHCOUNT_T_MAX, (int (*)(const void *, const void *)) strcmp, NULL)) == NULL) {
			status = CP_ERR_RESOURCE;
			break;
		}
	
		// Scan plug-in directories for available plug-ins 
		lnode = list_first(context->plugin_dirs);
		while (lnode != NULL) {
			const char *dir_path;
			DIR *dir;
			
			dir_path = lnode_get(lnode);
			dir = opendir(dir_path);
			if (dir != NULL) {
				int dir_path_len;
				struct dirent *de;
				
				dir_path_len = strlen(dir_path);
				if (dir_path[dir_path_len - 1] == CP_FNAMESEP_CHAR) {
					dir_path_len--;
				}
				errno = 0;
				while ((de = readdir(dir)) != NULL) {
					if (de->d_name[0] != '\0' && de->d_name[0] != '.') {
						int pdir_path_len = dir_path_len + 1 + strlen(de->d_name);
						cp_plugin_t *plugin;
						int s;
						hnode_t *hnode;

						// Allocate memory for plug-in descriptor path 
						if (pdir_path_size <= pdir_path_len) {
							char *new_pdir_path;
						
							if (pdir_path_size == 0) {
								pdir_path_size = 128;
							}
							while (pdir_path_size <= pdir_path_len) {
								pdir_path_size *= 2;
							}
							new_pdir_path = realloc(pdir_path, pdir_path_size * sizeof(char));
							if (new_pdir_path == NULL) {
								cpi_errorf(context, _("Could not check possible plug-in location %s%c%s due to insufficient system resources."), dir_path, CP_FNAMESEP_CHAR, de->d_name);
								status = CP_ERR_RESOURCE;
								// continue loading plug-ins from other directories 
								continue;
							}
							pdir_path = new_pdir_path;
						}
					
						// Construct plug-in descriptor path 
						strcpy(pdir_path, dir_path);
						pdir_path[dir_path_len] = CP_FNAMESEP_CHAR;
						strcpy(pdir_path + dir_path_len + 1, de->d_name);
							
						// Try to load a plug-in 
						s = load_plugin(context, pdir_path, &plugin);
						if (s != CP_OK) {
							status = s;
							// continue loading plug-ins from other directories 
							continue;
						}
					
						// Insert plug-in to the list of available plug-ins 
						if ((hnode = hash_lookup(avail_plugins, plugin->identifier)) != NULL) {
							cp_plugin_t *plugin2 = hnode_get(hnode);						
							if (cpi_version_cmp(plugin2->version, plugin->version, 4) < 0) {
								hash_delete_free(avail_plugins, hnode);
								cpi_free_plugin(plugin2);
								hnode = NULL;
							}
						}
						if (hnode == NULL) {
							if (!hash_alloc_insert(avail_plugins, plugin->identifier, plugin)) {
								cpi_errorf(context, _("Plug-in %s version %s could not be loaded due to insufficient system resources."), plugin->identifier, plugin->version);
								cpi_free_plugin(plugin);
								status = CP_ERR_RESOURCE;
								// continue loading plug-ins from other directories 
								continue;
							}
						}
						
					}
					errno = 0;
				}
				if (errno) {
					cpi_errorf(context, _("Could not read plug-in directory %s: %s"), dir_path, strerror(errno));
					status = CP_ERR_IO;
					// continue loading plug-ins from other directories 
				}
				closedir(dir);
			} else {
				cpi_errorf(context, _("Could not open plug-in directory %s: %s"), dir_path, strerror(errno));
				status = CP_ERR_IO;
				// continue loading plug-ins from other directories 
			}
			
			lnode = list_next(context->plugin_dirs, lnode);
		}
		
		// Copy the list of started plug-ins, if necessary 
		if ((flags & CP_LP_RESTART_ACTIVE)
			&& (flags & (CP_LP_UPGRADE | CP_LP_STOP_ALL_ON_INSTALL))) {
			int s, i;

			if ((plugins = cp_get_plugins(context, &s, NULL)) == NULL) {
				status = s;
				break;
			}
			if ((started_plugins = list_create(LISTCOUNT_T_MAX)) == NULL) {
				status = CP_ERR_RESOURCE;
				break;
			}
			for (i = 0; plugins[i] != NULL; i++) {
				cp_plugin_state_t state;
				
				state = cp_get_plugin_state(context, plugins[i]->identifier);
				if (state == CP_PLUGIN_STARTING || state == CP_PLUGIN_ACTIVE) {
					char *pid;
				
					if ((pid = cpi_strdup(plugins[i]->identifier)) == NULL) {
						status = CP_ERR_RESOURCE;
						break;
					}
					if ((lnode = lnode_create(pid)) == NULL) {
						free(pid);
						status = CP_ERR_RESOURCE;
						break;
					}
					list_append(started_plugins, lnode);
				}
			}
			cp_release_plugins(plugins);
			plugins = NULL;
		}
		
		// Install/upgrade plug-ins 
		hash_scan_begin(&hscan, avail_plugins);
		while ((hnode = hash_scan_next(&hscan)) != NULL) {
			cp_plugin_t *plugin;
			cp_plugin_state_t state;
			int s;
			
			plugin = hnode_get(hnode);
			state = cp_get_plugin_state(context, plugin->identifier);
			
			// Unload the installed plug-in if it is to be upgraded 
			if (state >= CP_PLUGIN_INSTALLED
				&& (flags & CP_LP_UPGRADE)) {
				if ((flags & (CP_LP_STOP_ALL_ON_UPGRADE | CP_LP_STOP_ALL_ON_INSTALL))
					&& !plugins_stopped) {
					plugins_stopped = 1;
					s = cp_stop_all_plugins(context);
					if (s != CP_OK) {
						status = s;
						break;
					}
				}
				s = cp_unload_plugin(context, plugin->identifier);
				if (s != CP_OK) {
					status = s;
					break;
				}
				state = CP_PLUGIN_UNINSTALLED;
				assert(cp_get_plugin_state(context, plugin->identifier) == state);
			}
			
			// Install the plug-in, if to be installed 
			if (state == CP_PLUGIN_UNINSTALLED) {
				if ((flags & CP_LP_STOP_ALL_ON_INSTALL) && !plugins_stopped) {
					plugins_stopped = 1;
					s = cp_stop_all_plugins(context);
					if (s != CP_OK) {
						status = s;
						break;
					}
				}
				cpi_install_plugin(context, plugin, 0);
				hash_scan_delfree(avail_plugins, hnode);
			}
		}
		
		// Restart stopped plug-ins if necessary 
		if (started_plugins != NULL) {
			lnode = list_first(started_plugins);
			while (lnode != NULL) {
				char *pid;
				int s;
				
				pid = lnode_get(lnode);
				s = cp_start_plugin(context, pid);
				if (s != CP_OK) {
					status = s;
				}
				lnode = list_next(started_plugins, lnode);
			}
		}
		
	} while (0);
	cpi_unlock_context(context);

	// Release resources 
	if (pdir_path != NULL) {
		free(pdir_path);
	}
	if (avail_plugins != NULL) {
		hscan_t hscan;
		hnode_t *hnode;
		
		hash_scan_begin(&hscan, avail_plugins);
		while ((hnode = hash_scan_next(&hscan)) != NULL) {
			cp_plugin_t *p = hnode_get(hnode);
			hash_scan_delfree(avail_plugins, hnode);
			cpi_free_plugin(p);
		}
		hash_destroy(avail_plugins);
	}
	if (started_plugins != NULL) {
		list_process(started_plugins, NULL, cpi_process_free_ptr);
		list_destroy(started_plugins);
	}
	if (plugins != NULL) {
		cp_release_plugins(plugins);
	}

	// Error handling 
	switch (status) {
		case CP_OK:
			break;
		case CP_ERR_RESOURCE:
			cpi_error(context, _("Could not load plug-ins due to insufficient system resources."));
			break;
		default:
			cpi_error(context, _("Could not load plug-ins."));
			break;
	}
	
	return status;
}
