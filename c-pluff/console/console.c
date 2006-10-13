/*-------------------------------------------------------------------------
 * C-Pluff, a plug-in framework for C
 * Copyright 2006 Johannes Lehtinen
 *-----------------------------------------------------------------------*/

/* Core console logic */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include "../libcpluff/cpluff.h"
#include "console.h"


/* ------------------------------------------------------------------------
 * Definitions
 * ----------------------------------------------------------------------*/

/** The maximum number of contexts supported */
#define MAX_NUM_CONTEXTS 8


/* ------------------------------------------------------------------------
 * Data types
 * ----------------------------------------------------------------------*/

typedef struct flag_info_t {
	
	/** The name of the flag */
	char *name;
	
	/** The value of the flag */
	int value;
	
} flag_info_t;


/* ------------------------------------------------------------------------
 * Function declarations
 * ----------------------------------------------------------------------*/

/* Function declarations for command implementations */
static void cmd_help(int argc, char *argv[]);
static void cmd_create_context(int argc, char *argv[]);
static void cmd_select_context(int argc, char *argv[]);
static void cmd_destroy_context(int argc, char *argv[]);
static void cmd_add_plugin_dir(int argc, char *argv[]);
static void cmd_remove_plugin_dir(int argc, char *argv[]);
static void cmd_load_plugin(int argc, char *argv[]);
static void cmd_load_plugins(int argc, char *argv[]);
static void cmd_exit(int argc, char *argv[]);

/* ------------------------------------------------------------------------
 * Variables
 * ----------------------------------------------------------------------*/

/** Known plug-in contexts */
cp_context_t *contexts[MAX_NUM_CONTEXTS];

/** The index of the currently active context or -1 if none */
static int active_context = -1;

/** The index of the next created context, or -1 if no more room */
static int next_context = 0;

/** The available commands */
const command_info_t commands[] = {
	{ N_("help"), N_("displays command help"), cmd_help },
	{ N_("create-context"), N_("creates a new plug-in context"), cmd_create_context },
	{ N_("select-context"), N_("selects a plug-in context as the active context"), cmd_select_context },
	{ N_("destroy-context"), N_("destroys the selected plug-in context"), cmd_destroy_context },
	{ N_("add-plugin-dir"), N_("registers a plug-in directory"), cmd_add_plugin_dir },
	{ N_("remove-plugin-dir"), N_("unregisters a plug-in directory"), cmd_remove_plugin_dir },
	{ N_("load-plugin"), N_("loads a plug-in from the specified path"), cmd_load_plugin },
	{ N_("load-plugins"), N_("loads plug-ins from the registered plug-in directories"), cmd_load_plugins },
	{ N_("exit"), N_("quits the program"), cmd_exit },
	{ N_("quit"), N_("quits the program"), cmd_exit },
	{ NULL, NULL, NULL }
};

/** The available load flags */
const flag_info_t load_flags[] = {
	{ N_("upgrade"), CP_LP_UPGRADE },
	{ N_("stop-all-on-upgrade"), CP_LP_STOP_ALL_ON_UPGRADE },
	{ N_("stop-all-on-install"), CP_LP_STOP_ALL_ON_INSTALL },
	{ N_("restart-active"), CP_LP_RESTART_ACTIVE },
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

static int destroy_context(int ci) {
	if (contexts[ci] != NULL) {
		cp_destroy_context(contexts[ci]);
		contexts[ci] = NULL;
		noticef(_("Destroyed plug-in context %d."), ci);
		return 0;
	} else {
		return -1;
	}
}

static void cmd_exit(int argc, char *argv[]) {
	int i;
	
	/* Destroy all plug-in contexts */
	for (i = 0; i < MAX_NUM_CONTEXTS; i++) {
		destroy_context(i);
	}
	
	/* Exit program */
	exit(0);
}

static void cmd_help(int argc, char *argv[]) {
	int i;
	
	notice(_("The following commands are available:"));
	for (i = 0; commands[i].name != NULL; i++) {
		noticef(N_("  %s - %s"), commands[i].name, _(commands[i].description));
	}
}

static void unrecognized_arguments(void) {
	error(_("Unrecognized arguments were given to the command."));
}

static void no_active_context(void) {	
	error(_("There is no active plug-in context."));
}

static void error_handler(cp_context_t *context, const char *msg) {
	int i;

	for (i = 0; i < MAX_NUM_CONTEXTS && context != contexts[i]; i++);
	if (i == MAX_NUM_CONTEXTS) {
		i = next_context;
	}
	errorf(_("[context %d]: %s"), i, msg);
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

static void event_listener(cp_context_t *context, const cp_plugin_event_t *event) {
	int i;
	
	for (i = 0; i < MAX_NUM_CONTEXTS && context != contexts[i]; i++);
	noticef(_("EVENT [context %d]: Plug-in %s changed from %s to %s."),
		i,
		event->plugin_id,
		state_to_string(event->old_state),
		state_to_string(event->new_state));
}

static void cmd_create_context(int argc, char *argv[]) {
	if (argc == 1) {
		int i;
		int status;
		
		/* Check that there is space for a new context */
		if (next_context == -1) {
			error(_("Maximum number of plug-in contexts in use."));
			return;
		}

		/* Create a new context */		
		if ((contexts[next_context] = cp_create_context(error_handler, &status)) == NULL) {
			errorf(_("cp_create_context failed with error code %d."), status);
			return;
		}
		if ((status = cp_add_event_listener(contexts[next_context], event_listener)) != CP_OK) {
			errorf(_("cp_add_event_listener failed with error code %d."), status);
			cp_destroy_context(contexts[next_context]);
			contexts[next_context] = NULL;
			return;
		}
		active_context = next_context;
		noticef(_("Created plug-in context %d."), active_context);
		
		/* Find the index for the next context */
		i = next_context;
		do {
			if (++next_context >= MAX_NUM_CONTEXTS) {
				next_context = 0;
			}
			if (contexts[next_context] == NULL) {
				break;
			}
		} while (next_context != i);
		if (next_context == i) {
			next_context = -1;
		}
		
	} else {
		unrecognized_arguments();
	}
}

static void print_avail_contexts(void) {
	int i, first = 1;
	
	for (i = 0; i < MAX_NUM_CONTEXTS; i++) {
		if (contexts[i] != NULL) {
			if (first) {
				notice(_("Available plug-in contexts are:"));
				first = 0;
			}
			noticef(N_("  %d"), i);
		}
	}
	if (first) {
		notice(_("There are no plug-in contexts available."));
	}
}

static int choose_context(const char *ctx) {
	int i = atoi(ctx);
	
	if (i < 0 || i >= MAX_NUM_CONTEXTS || contexts[i] == NULL) {
		error(_("No such plug-in context."));
		return -1;
	} else {
		return i;
	}
}

static void cmd_select_context(int argc, char *argv[]) {
	if (argc == 1) {
		error(_("Usage: select-context <context>"));
		print_avail_contexts();
	} else if (argc == 2) {
		int i = choose_context(argv[1]);

		if (i != -1) {
			active_context = i;
			noticef(_("Selected plug-in context %d."), active_context);
		}
	} else {
		unrecognized_arguments();
	}
}

static void cmd_destroy_context(int argc, char *argv[]) {
	int ci;
	
	if (argc == 1) {
		if (active_context == -1) {
			no_active_context();
			return;
		}
		ci = active_context;
	} else if (argc == 2) {
		if ((ci = choose_context(argv[1])) == -1) {
			return;
		}
	} else {
		unrecognized_arguments();
		return;
	}
	
	/* Destroy the context */
	destroy_context(ci);
	
	/* Choose the next index if necessary */
	if (next_context == -1) {
		next_context = ci;
	}
	
	/* Choose the new active context if necessary */
	if (ci == active_context) {
		do {
			if (--active_context < 0) {
				active_context = MAX_NUM_CONTEXTS - 1;
			}
			if (contexts[active_context] != NULL) {
				break;
			}
		} while (active_context != ci);
		if (active_context == ci) {
			active_context = -1;
		}
	}
}

static void cmd_add_plugin_dir(int argc, char *argv[]) {
	if (argc == 1) {
		error(_("Usage: add-plugin-dir <path>"));
	} else if (argc == 2) {
		int status;
		
		if (active_context == -1) {
			no_active_context();
			return;
		}
		if ((status = cp_add_plugin_dir(contexts[active_context], argv[1])) != CP_OK) {
			errorf(_("cp_add_plugin_dir failed with error code %d."), status);
			return;
		}
		noticef(_("Registered plug-in directory %s for context %d."), argv[1], active_context);
	} else {
		unrecognized_arguments();
	}
}

static void cmd_remove_plugin_dir(int argc, char *argv[]) {
	if (argc == 1) {
		error(_("Usage: remove-plugin-dir <path>"));
	} else if (argc == 2) {
		if (active_context == -1) {
			no_active_context();
			return;
		}
		cp_remove_plugin_dir(contexts[active_context], argv[1]);
		noticef(_("Unregistered plug-in directory %s from context %d."), argv[1], active_context);
	} else {
		unrecognized_arguments();
	}
}

static void cmd_load_plugin(int argc, char *argv[]) {
	if (argc == 1) {
		error(_("Usage: load-plugin <path>"));
		return;
	} else if (argc == 2) {
		cp_plugin_t *plugin;
		int status;
		
		if (active_context == -1) {
			no_active_context();
			return;
		}
		if ((plugin = cp_load_plugin(contexts[active_context], argv[1], &status)) == NULL) {
			errorf(_("cp_load_plugin failed with error code %d."), status);
			return;
		}
		noticef(_("Loaded plug-in %s into plug-in context %d."), plugin->identifier, active_context);
		cp_release_plugin(plugin);
	} else {
		unrecognized_arguments();
	}
}

static void cmd_load_plugins(int argc, char *argv[]) {
	int flags = 0;
	int status;
	int i;
	
	if (active_context == -1) {
		no_active_context();
		return;
	}
	
	/* Set flags */
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
			error(_("Usage: cp-load-plugins [<flag> [<flag>]...]"));
			notice(_("Available flags are:"));
			for (j = 0; load_flags[j].name != NULL; j++) {
				noticef(N_("  %s"), load_flags[j].name);
			}
			return;
		}
	}
	
	if ((status = cp_load_plugins(contexts[active_context], flags)) != CP_OK) {
		errorf(_("cp_load_plugins failed with error code %d."), status);
		return;
	}
	notice(_("Plug-ins loaded."));
}

int main(int argc, char *argv[]) {
	const cp_implementation_info_t *ii;
	char *prompt_no_context, *prompt_context;
	int i;

	/* Initialize C-Pluff library (also initializes gettext) */
	cp_init();
	
	/* Display startup information */
	ii = cp_get_implementation_info();
	noticef(_("%s console %s [%d:%d:%d]"), PACKAGE_NAME, PACKAGE_VERSION, CP_API_VERSION, CP_API_REVISION, CP_API_AGE);
	if (ii->multi_threading_type != NULL) {
		noticef(_("%s library %s [%d:%d:%d] for %s with %s threads"),
			PACKAGE_NAME, ii->release_version, ii->api_version,
			ii->api_revision, ii->api_age, ii->host_type,
			ii->multi_threading_type);
	} else {
		noticef(_("%s library %s [%d:%d:%d] for %s without threads"),
			PACKAGE_NAME, ii->release_version, ii->api_version,
			ii->api_revision, ii->api_age, ii->host_type);
	}
	notice(_("Type \"help\" for help on available commands."));

	/* Initialize context array */
	for (i = 0; i < MAX_NUM_CONTEXTS; i++) {
		contexts[i] = NULL;
	}

	/* Command line loop */
	cmdline_init();
	prompt_no_context = _("[no context] > ");
	prompt_context = _("[context %d] > ");
	while (1) {
		char prompt[32];
		char *cmdline;
		int argc;
		char **argv;
		
		/* Get command line */
		if (active_context != -1) {
			snprintf(prompt, sizeof(prompt), prompt_context, active_context);
			prompt[sizeof(prompt)/sizeof(char) - 1] = '\0';
			cmdline = cmdline_input(prompt);
		} else {
			cmdline = cmdline_input(prompt_no_context);
		}
		if (cmdline == NULL) {
			putchar('\n');
			cmdline = N_("exit");
		}
		
		/* Parse command line */
		argc = cmdline_parse(cmdline, &argv);
		if (argc <= 0) {
			continue;
		}
		
		/* Choose command */
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
