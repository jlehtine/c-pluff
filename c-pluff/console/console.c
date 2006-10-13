/*-------------------------------------------------------------------------
 * C-Pluff, a plug-in framework for C
 * Copyright 2006 Johannes Lehtinen
 *-----------------------------------------------------------------------*/

/* Core console logic */

#include <stdio.h>
#include <stdlib.h>
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
 * Variables
 * ----------------------------------------------------------------------*/

/** Known plug-in contexts */
cp_context_t *contexts[MAX_NUM_CONTEXTS] = {
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL
};

/** The index of the currently active context or -1 if none */
static int active_context = -1;

/** The index of the next created context, or -1 if no more room */
static int next_context = 0;

/** The available command names */
const char *commands[] = {
	N_("help"),
	N_("create-context"),
	N_("select-context"),
	N_("destroy-context"),
	N_("load-plugin"),
	N_("exit"),
	N_("quit"),
	NULL
};


/* ------------------------------------------------------------------------
 * Function definitions
 * ----------------------------------------------------------------------*/

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
		fputs(_("ERROR: Command has too many arguments.\n"), stderr);
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
		printf(_("Destroyed plug-in context %d.\n"), ci);
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
	fputs(_("The following commands are available:\n"
		"  license - displays license information\n"
		"  help - displays command help\n"
		"  create-context - creates a new plug-in context\n"
		"  select-context - selects a plug-in context as the active context\n"
		"  destroy-context - destroys the selected plug-in context\n"
		"  load-plugin - loads a plug-in from the specified path\n"
	), stdout);
}

static void unrecognized_arguments(void) {
	fputs(_("ERROR: Unrecognized arguments.\n"), stderr);
}

static void no_active_context(void) {
	fputs(_("ERROR: There is no active plug-in context.\n"), stderr);
}

static void error_handler(cp_context_t *context, const char *msg) {
	int i;

	for (i = 0; i < MAX_NUM_CONTEXTS && context != contexts[i]; i++);
	if (i == MAX_NUM_CONTEXTS) {
		i = next_context;
	}
	fprintf(stderr, _("ERROR [context %d]: %s\n"), i, msg);
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
	printf(_("EVENT [context %d]: Plug-in %s changed from %s to %s.\n"),
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
			fputs(_("ERROR: Maximum number of plug-in contexts in use.\n"), stderr);
			return;
		}

		/* Create a new context */		
		if ((contexts[next_context] = cp_create_context(error_handler, &status)) == NULL) {
			fprintf(stderr, _("ERROR: cp_create_context failed with error code %d.\n"), status);
			return;
		}
		if ((status = cp_add_event_listener(contexts[next_context], event_listener)) != CP_OK) {
			fprintf(stderr, _("ERROR: cp_add_event_listener failed with error code %d.\n"), status);
			cp_destroy_context(contexts[next_context]);
			contexts[next_context] = NULL;
			return;
		}
		active_context = next_context;
		printf(_("Created plug-in context %d.\n"), active_context);
		
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
			if (!first) {
				fputs(_(", "), stdout);
			} else {
				fputs(_("Available plug-in contexts are: "), stdout);
				first = 0;
			}
			printf(N_("%d"), i);
		}
	}
	if (first) {
		fputs(_("There are no plug-in contexts available.\n"), stdout);
	} else {
		fputs(_(".\n"), stdout);
	}
}

static int choose_context(const char *ctx) {
	int i = atoi(ctx);
	
	if (i < 0 || i >= MAX_NUM_CONTEXTS || contexts[i] == NULL) {
		fputs(_("ERROR: No such plug-in context.\n"), stderr);
		return -1;
	} else {
		return i;
	}
}

static void cmd_select_context(int argc, char *argv[]) {
	if (argc == 1) {
		fputs(_("ERROR: Usage: select-context [context]\n"), stderr);
		print_avail_contexts();
	} else if (argc == 2) {
		int i = choose_context(argv[1]);

		if (i != -1) {
			active_context = i;
			printf(_("Selected plug-in context %d.\n"), active_context);
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

static void cmd_load_plugin(int argc, char *argv[]) {
	if (argc == 1) {
		fputs(_("ERROR: Usage: load-plugin [path]\n"), stderr);
		return;
	} else if (argc == 2) {
		cp_plugin_t *plugin;
		int status;
		
		if (active_context == -1) {
			no_active_context();
			return;
		}
		if ((plugin = cp_load_plugin(contexts[active_context], argv[1], &status)) == NULL) {
			fprintf(stderr, _("ERROR: cp_load_plugin failed with error code %d.\n"), status);
			return;
		}
		printf(_("Loaded plug-in %s into plug-in context %d.\n"), plugin->identifier, active_context);
		cp_release_plugin(plugin);
	} else {
		unrecognized_arguments();
	}
}

int main(int argc, char *argv[]) {
	const cp_implementation_info_t *ii;
	char *prompt_no_context, *prompt_context;

	/* Initialize C-Pluff library (also initializes gettext) */
	cp_init();
	
	/* Display startup information */
	ii = cp_get_implementation_info();
	printf(_("%s console %s [%d:%d:%d]\n"), PACKAGE_NAME, PACKAGE_VERSION, CP_API_VERSION, CP_API_REVISION, CP_API_AGE);
	if (ii->multi_threading_type != NULL) {
		printf(_("%s library %s [%d:%d:%d] for %s with %s threads\n"),
			PACKAGE_NAME, ii->release_version, ii->api_version,
			ii->api_revision, ii->api_age, ii->host_type,
			ii->multi_threading_type);
	} else {
		printf(_("%s library %s [%d:%d:%d] for %s without threads\n"),
			PACKAGE_NAME, ii->release_version, ii->api_version,
			ii->api_revision, ii->api_age, ii->host_type);
	}
	fputs(_("Type \"help\" for help on available commands.\n"), stdout);

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
			cmd_exit(0, NULL);
		}
		
		/* Parse command line */
		argc = cmdline_parse(cmdline, &argv);
		if (argc <= 0) {
			continue;
		}
		
		/* Choose command */
		if (!strcmp(argv[0], "exit") || !strcmp(argv[0], "quit")) {
			cmd_exit(argc, argv);
		} else if (!strcmp(argv[0], "help")) {
			cmd_help(argc, argv);
		} else if (!strcmp(argv[0], "create-context")) {
			cmd_create_context(argc, argv);
		} else if (!strcmp(argv[0], "select-context")) {
			cmd_select_context(argc, argv);
		} else if (!strcmp(argv[0], "destroy-context")) {
			cmd_destroy_context(argc, argv);
		} else if (!strcmp(argv[0], "load-plugin")) {
			cmd_load_plugin(argc, argv);
		} else {
			fprintf(stderr, _("ERROR: Unknown command %s\n"), argv[0]);
		}
	}
}
