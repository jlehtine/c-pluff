/*-------------------------------------------------------------------------
 * C-Pluff, a plug-in framework for C
 * Copyright 2006 Johannes Lehtinen
 *-----------------------------------------------------------------------*/

// Core console logic 

#include "console.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#ifdef HAVE_GETTEXT
#include <locale.h>
#endif
#include <assert.h>
#include <cpluff.h>


/* ------------------------------------------------------------------------
 * Function declarations
 * ----------------------------------------------------------------------*/

// Function declarations for command implementations 
static void cmd_help(int argc, char *argv[]);
static void cmd_register_pcollection(int argc, char *argv[]);
static void cmd_unregister_pcollection(int argc, char *argv[]);
static void cmd_unregister_pcollections(int argc, char *argv[]);
static void cmd_load_plugin(int argc, char *argv[]);
static void cmd_scan_plugins(int argc, char *argv[]);
static void cmd_list_plugins(int argc, char *argv[]);
static void cmd_show_plugin_info(int argc, char *argv[]);
static void cmd_list_ext_points(int argc, char *argv[]);
static void cmd_list_extensions(int argc, char *argv[]);
static void cmd_set_context_args(int argc, char *argv[]);
static void cmd_start_plugin(int argc, char *argv[]);
static void cmd_run_plugins_step(int argc, char *argv[]);
static void cmd_run_plugins(int argc, char *argv[]);
static void cmd_stop_plugin(int argc, char *argv[]);
static void cmd_stop_plugins(int argc, char *argv[]);
static void cmd_uninstall_plugin(int argc, char *argv[]);
static void cmd_uninstall_plugins(int argc, char *argv[]);
static void cmd_exit(int argc, char *argv[]);

/* ------------------------------------------------------------------------
 * Variables
 * ----------------------------------------------------------------------*/

/// The plug-in context
static cp_context_t *context;

/// The available commands 
const command_info_t commands[] = {
	{ "help", N_("displays command help"), cmd_help },
	{ "register-collection", N_("registers a plug-in collection"), cmd_register_pcollection },
	{ "unregister-collection", N_("unregisters a plug-in collection"), cmd_unregister_pcollection },
	{ "unregister-collections", N_("unregisters all plug-in collections"), cmd_unregister_pcollections },
	{ "load-plugin", N_("loads and installs a plug-in from the specified path"), cmd_load_plugin },
	{ "scan-plugins", N_("scans plug-ins in the registered plug-in collections"), cmd_scan_plugins },
	{ "set-context-args", N_("sets context startup arguments"), cmd_set_context_args },
	{ "start-plugin", N_("starts a plug-in"), cmd_start_plugin },
	{ "run-plugins-step", N_("runs one plug-in function"), cmd_run_plugins_step },
	{ "run-plugins", N_("runs plug-in functions until no further work to be done"), cmd_run_plugins },
	{ "stop-plugin", N_("stops a plug-in"), cmd_stop_plugin },
	{ "stop-plugins", N_("stops all plug-ins"), cmd_stop_plugins },
	{ "uninstall-plugin", N_("uninstalls a plug-in"), cmd_uninstall_plugin },
	{ "uninstall-plugins", N_("uninstalls all plug-ins"), cmd_uninstall_plugins },
	{ "list-plugins", N_("lists the installed plug-ins"), cmd_list_plugins },
	{ "list-ext-points", N_("lists the installed extension points"), cmd_list_ext_points },
	{ "list-extensions", N_("lists the installed extensions"), cmd_list_extensions },
	{ "show-plugin-info", N_("shows static plug-in information"), cmd_show_plugin_info },
	{ "quit", N_("quits the program"), cmd_exit },
	{ "exit", N_("quits the program"), cmd_exit },
	{ NULL, NULL, NULL }
};

/// The available load flags 
const flag_info_t load_flags[] = {
	{ "upgrade", CP_LP_UPGRADE },
	{ "stop-all-on-upgrade", CP_LP_STOP_ALL_ON_UPGRADE },
	{ "stop-all-on-install", CP_LP_STOP_ALL_ON_INSTALL },
	{ "restart-active", CP_LP_RESTART_ACTIVE },
	{ NULL, -1 }
};


/* ------------------------------------------------------------------------
 * Function definitions
 * ----------------------------------------------------------------------*/

/**
 * Prints an error message out.
 * 
 * @param msg the error message
 */
static void error(const char *msg) {
	fprintf(stderr, _("ERROR: %s\n"), msg);
}

/**
 * Prints a formatted error message out.
 * 
 * @param msg the formatting rule
 * @param ... the parameters
 */
static void errorf(const char *msg, ...) {
	va_list vl;
	char buffer[256];

	va_start(vl, msg);
	vsnprintf(buffer, sizeof(buffer), msg, vl);
	buffer[sizeof(buffer)/sizeof(char) - 1] = '\0';
	va_end(vl);
	error(buffer);
}

/**
 * Prints a notice out.
 * 
 * @param msg the message
 */
static void notice(const char *msg) {
	puts(msg);
}

/**
 * Prints a formatted notice out.
 * 
 * @param msg the message
 * @param ... the parameters
 */
static void noticef(const char *msg, ...) {
	va_list vl;
	char buffer[256];

	va_start(vl, msg);
	vsnprintf(buffer, sizeof(buffer), msg, vl);
	buffer[sizeof(buffer)/sizeof(char) - 1] = '\0';
	va_end(vl);
	notice(buffer);
}

/**
 * Parses a command line (in place) into white-space separated elements.
 * Returns the number of elements and the pointer to argument table including
 * command and arguments. The argument table is valid until the next call
 * to this function.
 * 
 * @param cmdline the command line to be parsed
 * @param argv pointer to the argument table is stored here
 * @return the number of command line elements, or -1 on failure
 */
static int cmdline_parse(char *cmdline, char **argv[]) {
	static char *sargv[16];
	int i, argc;
			
	for (i = 0; isspace(cmdline[i]); i++);
	for (argc = 0; cmdline[i] != '\0' && argc < 16; argc++) {
		sargv[argc] = cmdline + i;
		while (cmdline[i] != '\0' && !isspace(cmdline[i])) {
			i++;
		}
		if (cmdline[i] != '\0') {
			cmdline[i++] = '\0';
			while (isspace(cmdline[i])) {
				i++;
			}
		}
	}
	if (cmdline[i] != '\0') {
		error(_("Command has too many arguments."));
		return -1;
	} else {
		*argv = sargv;
		return argc;
	}
}

static void cmd_exit(int argc, char *argv[]) {
	
	// Destroy C-Pluff framework 
	cp_destroy();
	
	// Exit program 
	exit(0);
}

static void cmd_help(int argc, char *argv[]) {
	int i;
	
	notice(_("The following commands are available:"));
	for (i = 0; commands[i].name != NULL; i++) {
		noticef("  %s - %s", commands[i].name, _(commands[i].description));
	}
}

static void logger(cp_log_severity_t severity, const char *msg, const char *apid, void *dummy) {
	char *prefix;
	
	switch (severity) {
		
		case CP_LOG_ERROR:
			if (apid != NULL) {
				errorf("%s: %s", apid, msg);
			} else {
				error(msg);
			}
			return;
			
		case CP_LOG_WARNING:
			prefix = "WARNING";
			break;
			
		case CP_LOG_DEBUG:
			prefix = "DEBUG";
			break;
			
		case CP_LOG_INFO:
			prefix = "INFO";
			break;
			
		default:
			prefix = "UNKNOWN";
			break;
	}
	if (apid != NULL) {
		noticef("%s: %s: %s", prefix, apid, msg);
	} else {
		noticef("%s: %s", prefix, msg);
	}
}

static char *state_to_string(cp_plugin_state_t state) {
	switch (state) {
		case CP_PLUGIN_UNINSTALLED:
			return "UNINSTALLED";
		case CP_PLUGIN_INSTALLED:
			return "INSTALLED";
		case CP_PLUGIN_RESOLVED:
			return "RESOLVED";
		case CP_PLUGIN_STARTING:
			return "STARTING";
		case CP_PLUGIN_STOPPING:
			return "STOPPING";
		case CP_PLUGIN_ACTIVE:
			return "ACTIVE";
		default:
			return "(unknown)";
	}
}

static void plugin_listener(const char *plugin_id, cp_plugin_state_t old_state, cp_plugin_state_t new_state, void *dummy) {
	noticef(_("PLUGIN EVENT: %s: %s -> %s"),
		plugin_id,
		state_to_string(old_state),
		state_to_string(new_state));
}

static void cmd_register_pcollection(int argc, char *argv[]) {
	cp_status_t status;
	
	if (argc != 2) {
		errorf(_("Usage: %s <path>"), argv[0]);
	} else if ((status = cp_register_pcollection(context, argv[1])) != CP_OK) {
		errorf(_("cp_register_pcollection failed with error code %d."), status);
	} else {
		noticef(_("Registered plug-in collection at %s."), argv[1]);
	}
}

static void cmd_unregister_pcollection(int argc, char *argv[]) {
	if (argc != 2) {
		errorf(_("Usage: %s <path>"), argv[0]);
	} else {
		cp_unregister_pcollection(context, argv[1]);
		noticef(_("Unregistered plug-in collection at %s."), argv[1]);
	}
}

static void cmd_unregister_pcollections(int argc, char *argv[]) {
	if (argc != 1) {
		errorf(_("Usage: %s"), argv[0]);
	} else {
		cp_unregister_pcollections(context);
		notice(_("Unregistered all plug-in collections."));
	}
}

static void cmd_load_plugin(int argc, char *argv[]) {
	cp_plugin_info_t *plugin;
	cp_status_t status;
		
	if (argc != 2) {
		errorf(_("Usage: %s <path>"), argv[0]);
	} else if ((plugin = cp_load_plugin_descriptor(context, argv[1], &status)) == NULL) {
		errorf(_("cp_load_plugin_descriptor failed with error code %d."), status);
	} else if ((status = cp_install_plugin(context, plugin)) != CP_OK) {
		errorf(_("cp_install_plugin failed with error code %d."), status);
		cp_release_info(plugin);
	} else {
		noticef(_("Installed plug-in %s."), plugin->identifier);
		cp_release_info(plugin);
	}
}

static void cmd_scan_plugins(int argc, char *argv[]) {
	int flags = 0;
	cp_status_t status;
	int i;
	
	// Set flags 
	for (i = 1; i < argc; i++) {
		int j;
		
		for (j = 0; load_flags[j].name != NULL; j++) {
			if (!strcmp(argv[i], load_flags[j].name)) {
				flags |= load_flags[j].value;
				break;
			}
		}
		if (load_flags[j].name == NULL) {
			errorf(_("Unknown flag %s."), argv[i]);
			errorf(_("Usage: %s [<flag>...]"), argv[0]);
			notice(_("Available flags are:"));
			for (j = 0; load_flags[j].name != NULL; j++) {
				noticef("  %s", load_flags[j].name);
			}
			return;
		}
	}
	
	if ((status = cp_scan_plugins(context, flags)) != CP_OK) {
		errorf(_("cp_load_plugins failed with error code %d."), status);
		return;
	}
	notice(_("Plug-ins loaded."));
}

static void cmd_list_plugins(int argc, char *argv[]) {
	cp_plugin_info_t **plugins;
	cp_status_t status;
	int i;

	if (argc != 1) {
		errorf(_("Usage: %s"), argv[0]);
	} else if ((plugins = cp_get_plugins_info(context, &status, NULL)) == NULL) {
		errorf(_("cp_get_plugins_info failed with error code %d."), status);
	} else {
		notice(_("Installed plug-ins:"));
		for (i = 0; plugins[i] != NULL; i++) {
			if (plugins[i]->name != NULL) {
				noticef("  %s %s %s \"%s\"",
					plugins[i]->identifier,
					plugins[i]->version != NULL ? plugins[i]->version : "<unversioned>",
					state_to_string(cp_get_plugin_state(context, plugins[i]->identifier)),
					plugins[i]->name
				);
			} else {
				noticef("  %s %s %s",
					plugins[i]->identifier,
					plugins[i]->version != NULL ? plugins[i]->version : "<unversioned>",
					state_to_string(cp_get_plugin_state(context, plugins[i]->identifier))
				);
			}
		}
		cp_release_info(plugins);
	}
}

static char *str_or_null(const char *str) {
	static char *buffer = NULL;
	static int buffer_size = 0;
	
	if (str != NULL) {
		int rs = strlen(str) + 3;
		int do_realloc = 0;

		while (buffer_size < rs) {
			if (buffer_size == 0) {
				buffer_size = 64;
			} else {
				buffer_size *= 2;
			}
			do_realloc = 1;
		}
		if (do_realloc) {
			if ((buffer = realloc(buffer, buffer_size * sizeof(char))) == NULL) {
				error(_("Insufficient memory."));
				abort();
			}
		}
		snprintf(buffer, buffer_size, "\"%s\"", str);
		buffer[buffer_size - 1] = '\0';
		return buffer;
	} else {
		return "NULL";
	}
}

static void show_plugin_info_import(cp_plugin_import_t *import) {
	noticef("    plugin_id = \"%s\",", import->plugin_id);
	noticef("    version = %s,", str_or_null(import->version));
	noticef("    optional = %d,", import->optional);
}

static void show_plugin_info_ext_point(cp_ext_point_t *ep) {
	assert(ep->plugin != NULL);
	noticef("    local_id = \"%s\",", ep->local_id);
	noticef("    identifier = \"%s\",", ep->identifier);
	noticef("    name = %s,", str_or_null(ep->name));
	noticef("    schema_path = %s,", str_or_null(ep->schema_path));
}

static void strcat_quote_xml(char *dst, const char *src, int is_attr) {
	char c;
	
	while (*dst != '\0')
		dst++;
	do {
		switch ((c = *(src++))) {
			case '&':
				strcpy(dst, "&amp;");
				dst += 5;
				break;
			case '<':
				strcpy(dst, "&lt;");
				dst += 4;
				break;
			case '>':
				strcpy(dst, "&gt;");
				dst += 4;
				break;
			case '"':
				if (is_attr) {
					strcpy(dst, "&quot;");
					dst += 6;
					break;
				}
			default:
				*(dst++) = c;
				break;
		}
	} while (c != '\0');
}

static int strlen_quoted_xml(const char *str,int is_attr) {
	int len = 0;
	int i;
	
	for (i = 0; str[i] != '\0'; i++) {
		switch (str[i]) {
			case '&':
				len += 5;
				break;
			case '<':
			case '>':
				len += 4;
				break;
			case '"':
				if (is_attr) {
					len += 6;
					break;
				}
			default:
				len++;
		}
	}
	return len;
}

static void show_plugin_info_cfg(cp_cfg_element_t *ce, int indent) {
	static char *buffer = NULL;
	static int buffer_size = 0;
	int do_realloc;
	int rs;
	int i;

	// Calculate the maximum required buffer size
	rs = 2 * strlen(ce->name) + 6 + indent;
	if (ce->value != NULL) {
		rs += strlen_quoted_xml(ce->value, 0);
	}
	for (i = 0; i < ce->num_atts; i++) {
		rs += strlen(ce->atts[2*i]);
		rs += strlen_quoted_xml(ce->atts[2*i + 1], 1);
		rs += 4;
	}
	
	// Enlarge buffer if necessary
	do_realloc = 0;
	while (buffer_size < rs) {
		if (buffer_size == 0) {
			buffer_size = 64;
		} else {
			buffer_size *= 2;
		}
		do_realloc = 1;
	}
	if (do_realloc) {
		if ((buffer = realloc(buffer, buffer_size * sizeof(char))) == NULL) {
			error(_("Insufficient memory."));
			abort();
		}
	}
	
	// Create the string
	for (i = 0; i < indent; i++) {
		buffer[i] = ' ';
	}
	buffer[i++] = '<';
	buffer[i] = '\0';
	strcat(buffer, ce->name);
	for (i = 0; i < ce->num_atts; i++) {
		strcat(buffer, " ");
		strcat(buffer, ce->atts[2*i]);
		strcat(buffer, "=\"");
		strcat_quote_xml(buffer, ce->atts[2*i + 1], 1);
		strcat(buffer, "\"");
	}
	if (ce->value != NULL || ce->num_children) {
		strcat(buffer, ">");
		if (ce->value != NULL) {
			strcat_quote_xml(buffer, ce->value, 0);
		}
		if (ce->num_children) {
			notice(buffer);
			for (i = 0; i < ce->num_children; i++) {
				show_plugin_info_cfg(ce->children + i, indent + 2);
			}
			for (i = 0; i < indent; i++) {
				buffer[i] = ' ';
			}
			buffer[i++] = '<';
			buffer[i++] = '/';
			buffer[i] = '\0';
			strcat(buffer, ce->name);
			strcat(buffer, ">");
		} else {
			strcat(buffer, "</");
			strcat(buffer, ce->name);
			strcat(buffer, ">");
		}
	} else {
		strcat(buffer, "/>");
	}
	notice(buffer);
}

static void show_plugin_info_extension(cp_extension_t *e) {
	assert(e->plugin != NULL);
	noticef("    name = %s,", str_or_null(e->name));
	noticef("    ext_point_id = \"%s\",", e->ext_point_id);
	noticef("    local_id = %s,", str_or_null(e->local_id));
	noticef("    identifier = %s,", str_or_null(e->identifier));
	notice("    configuration = {");
	show_plugin_info_cfg(e->configuration, 6);
	notice("    },");
}

static void cmd_show_plugin_info(int argc, char *argv[]) {
	cp_plugin_info_t *plugin;
	cp_status_t status;
	int i;
	
	if (argc != 2) {
		errorf(_("Usage: %s <plugin>"), argv[0]);
	} else if ((plugin = cp_get_plugin_info(context, argv[1], &status)) == NULL) {
		errorf(_("cp_get_plugin_info failed with error code %d."), status);
	} else {
		notice("{");
		noticef("  identifier = \"%s\",", plugin->identifier);
		noticef("  name = %s,", str_or_null(plugin->name));
		noticef("  version = %s,", str_or_null(plugin->version));
		noticef("  provider_name = %s,", str_or_null(plugin->provider_name));
		noticef("  abi_bw_compatibility = %s,", str_or_null(plugin->abi_bw_compatibility));
		noticef("  api_bw_compatibility = %s,", str_or_null(plugin->api_bw_compatibility));
		noticef("  plugin_path = %s,", str_or_null(plugin->plugin_path));
		noticef("  req_cpluff_version = %s,", str_or_null(plugin->req_cpluff_version));
		noticef("  num_imports = %u,", plugin->num_imports);
		if (plugin->num_imports) {
			notice("  imports = {{");
			for (i = 0; i < plugin->num_imports; i++) {
				if (i)
					notice("  }, {");
				show_plugin_info_import(plugin->imports + i);
			}
			notice("  }},");
		} else {
			notice("  imports = {},");
		}
		noticef("  runtime_lib_name = %s,", str_or_null(plugin->runtime_lib_name));
		noticef("  runtime_funcs_symbol = %s,", str_or_null(plugin->runtime_funcs_symbol));
		noticef("  num_ext_points = %u,", plugin->num_ext_points);
		if (plugin->num_ext_points) {
			notice("  ext_points = {{");
			for (i = 0; i < plugin->num_ext_points; i++) {
				if (i)
					notice("  }, {");
				show_plugin_info_ext_point(plugin->ext_points + i);
			}
			notice("  }},");
		} else {
			notice("  ext_points = {},");
		}
		noticef("  num_extensions = %u,", plugin->num_extensions);
		if (plugin->num_extensions) {
			notice("  extensions = {{");
			for (i = 0; i < plugin->num_extensions; i++) {
				if (i)
					notice("  }, {");
				show_plugin_info_extension(plugin->extensions + i);
			}
			notice("  }}");
		} else {
			notice("  extensions = {},");
		}
		notice("}");
		cp_release_info(plugin);
	}
}

static void cmd_list_ext_points(int argc, char *argv[]) {
	cp_ext_point_t **ext_points;
	cp_status_t status;
	int i;

	if (argc != 1) {
		errorf(_("Usage: %s"), argv[0]);
	} else if ((ext_points = cp_get_ext_points_info(context, &status, NULL)) == NULL) {
		errorf(_("cp_get_ext_points_info failed with error code %d."), status);
	} else {
		notice(_("Installed extension points:"));
		for (i = 0; ext_points[i] != NULL; i++) {
			if (ext_points[i]->name != NULL) {
				noticef("  %s \"%s\" (%s)",
					ext_points[i]->identifier,
					ext_points[i]->name,
					ext_points[i]->plugin->identifier
				);
			} else {
				noticef("  %s (%s)",
					ext_points[i]->identifier,
					ext_points[i]->plugin->identifier
				);
			}
		}
		cp_release_info(ext_points);
	}	
}

static void cmd_list_extensions(int argc, char *argv[]) {
	cp_extension_t **extensions;
	cp_status_t status;
	int i;

	if (argc != 1) {
		errorf(_("Usage: %s"), argv[0]);
	} else if ((extensions = cp_get_extensions_info(context, NULL, &status, NULL)) == NULL) {
		errorf(_("cp_get_extensions_info failed with error code %d."), status);
	} else {
		notice(_("Installed extensions:"));
		for (i = 0; extensions[i] != NULL; i++) {
			if (extensions[i]->name != NULL) {
				noticef("  %s \"%s\" (%s)",
					extensions[i]->identifier != NULL ? extensions[i]->identifier : _("<unidentified>"),
					extensions[i]->name,
					extensions[i]->plugin->identifier
				);
			} else {
				noticef("  %s (%s)",
					extensions[i]->identifier != NULL ? extensions[i]->identifier : _("<unidentified>"),
					extensions[i]->plugin->identifier
				);
			}
		}
		cp_release_info(extensions);
	}	
}

static char *my_strdup(const char *str) {
	char *dup;
	
	if ((dup = malloc((strlen(str) + 1) * sizeof(char))) != NULL) {
		strcpy(dup, str);
	}
	return dup; 
}

static char **argv_dup(int argc, char *argv[]) {
	char **dup;
	int i;
	
	if ((dup = malloc((argc + 1) * sizeof(char *))) == NULL) {
		return NULL;
	}
	dup[0] = "";
	for (i = 1; i < argc; i++) {
		if ((dup[i] = my_strdup(argv[i])) == NULL) {
			for (i--; i > 0; i--) {
				free(dup[i]);
			}
			free(dup);
			return NULL;
		}
	}
	dup[argc] = NULL;
	return dup;
}

static void cmd_set_context_args(int argc, char *argv[]) {
	char **ctx_argv;

	if (argc != 1) {
		errorf(_("Usage: %s [<arg>...]"), argv[0]);
	} else if ((ctx_argv = argv_dup(argc, argv)) == NULL) {
		error(_("Insufficient memory."));
	} else {
		cp_set_context_args(context, argc, ctx_argv);
		notice(_("Context startup arguments have been set."));
	}
}

static void cmd_start_plugin(int argc, char *argv[]) {
	cp_status_t status;
	
	if (argc != 2) {
		errorf(_("Usage: %s <plugin>"), argv[0]);
	} else if ((status = cp_start_plugin(context, argv[1])) != CP_OK) {
		errorf(_("cp_start_plugin failed with error code %d."), status);
	} else {
		noticef(_("Started plug-in %s."), argv[1]);
	}
}

static void cmd_run_plugins_step(int argc, char *argv[]) {
	
	if (argc != 1) {
		errorf(_("Usage: %s"), argv[0]);
	} else {
		int pending = cp_run_plugins_step(context);
		if (pending) {
			notice(_("Ran plug-ins for one step. There are pending run functions."));
		} else {
			notice(_("Ran plug-ins for one step. No more pending run functions."));
		}
	}
}

static void cmd_run_plugins(int argc, char *argv[]) {
	if (argc != 1) {
		errorf(_("Usage: %s"), argv[0]);
	} else {
		cp_run_plugins(context);
		notice(_("Ran plug-ins. No more pending run functions."));
	}
}

static void cmd_stop_plugin(int argc, char *argv[]) {
	cp_status_t status;
	
	if (argc != 2) {
		errorf(_("Usage: %s <plugin>"), argv[0]);
	} else if ((status = cp_stop_plugin(context, argv[1])) != CP_OK) {
		errorf(_("cp_stop_plugin failed with error code %d."), status);
	} else {
		noticef(_("Stopped plug-in %s."), argv[1]);
	}
}

static void cmd_stop_plugins(int argc, char *argv[]) {
	if (argc != 1) {
		errorf(_("Usage: %s"), argv[0]);
	} else {
		cp_stop_plugins(context);
		notice(_("Stopped all plug-ins."));
	}
}

static void cmd_uninstall_plugin(int argc, char *argv[]) {
	cp_status_t status;
	
	if (argc != 2) {
		errorf(_("Usage: %s <plugin>"), argv[0]);
	} else if ((status = cp_uninstall_plugin(context, argv[1])) != CP_OK) {
		errorf(_("cp_uninstall_plugin failed with error code %d."), status);
	} else {
		noticef(_("Uninstalled plug-in %s."), argv[1]);
	}
}

static void cmd_uninstall_plugins(int argc, char *argv[]) {
	if (argc != 1) {
		errorf(_("Usage: %s"), argv[0]);
	} else {
		cp_uninstall_plugins(context);
		notice(_("Uninstalled all plug-ins."));
	}
}

int main(int argc, char *argv[]) {
	char *prompt;
	int i;
	cp_status_t status;

	// Gettext initialization 
#ifdef HAVE_GETTEXT
	setlocale(LC_ALL, "");
	bindtextdomain(PACKAGE, CP_DATADIR CP_FNAMESEP_STR "locale");
	textdomain(PACKAGE);
#endif

	// Display startup information 
	noticef(
		/* TRANSLATORS: This is the version string displayed on startup. */
		_("C-Pluff console, version %s"), PACKAGE_VERSION);
	noticef(
		/* TRANSLATORS: This is the version string displayed on startup. */
		_("C-Pluff framework, version %s for %s"),
		cp_get_version(), cp_get_host_type());
	notice(_("Type \"help\" for help on available commands."));

	// Initialize C-Pluff library 
	if ((status = cp_init()) != CP_OK) {
		errorf(_("cp_init failed with error code %d."), status);
		exit(1);
	}
	
	// Create a plug-in context
	context = cp_create_context(&status);
	if (context == NULL) {
		errorf(_("cp_create_context failed with error code %d."), status);
		exit(1);
	}

	// Initialize logging
	cp_register_logger(context, logger, NULL, CP_LOG_DEBUG);

	// Initialize plug-in listener
	cp_register_plistener(context, plugin_listener, NULL);

	// Command line loop 
	cmdline_init();
	
	/* TRANSLATORS: This is the input prompt for cpluff-console. */
	prompt = _("> ");
	
	while (1) {
		char prompt[32];
		char *cmdline;
		int argc;
		char **argv;
		
		// Get command line 
		cmdline = cmdline_input(prompt);
		if (cmdline == NULL) {
			putchar('\n');
			cmdline = "exit";
		}
		
		// Parse command line 
		argc = cmdline_parse(cmdline, &argv);
		if (argc <= 0) {
			continue;
		}
		
		// Choose command 
		for (i = 0; commands[i].name != NULL; i++) {
			if (!strcmp(argv[0], commands[i].name)) {
				commands[i].implementation(argc, argv);
				break;
			}
		}
		if (commands[i].name == NULL) {
			errorf(_("Unknown command %s."), argv[0]);
		}
	}
}
