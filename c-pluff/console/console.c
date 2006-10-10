/*-------------------------------------------------------------------------
 * C-Pluff, a plug-in framework for C
 * Copyright 2005-2006 Johannes Lehtinen
 *-----------------------------------------------------------------------*/

/* Core console logic */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "console.h"

/* ------------------------------------------------------------------------
 * Variables
 * ----------------------------------------------------------------------*/

/* The index of the currently active context or -1 if none */
static int active_context = -1;

/* The available command names */
static const char *commands[] = {
	N_("help"),
	N_("copyright"),
	N_("license"),
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

static void cmd_exit(int argc, char *argv[]) {
	// TODO
	exit(0);
}

static void cmd_help(int argc, char *argv[]) {
	int i;
	
	fputs(_("The following commands are available:\n"), stdout);
	for (i = 0; commands[i] != NULL; i++) {
		printf(N_("  %s\n"), commands[i]);
	}
}

static void cmd_copyright(int argc, char *argv[]) {
	fputs(N_(PACKAGE_STRING ", " CP_COPYRIGHT "\n"), stdout);
}

int main(int argc, char *argv[]) {
	char *prompt_no_context, *prompt_context;

	/* Initialize gettext */
	bindtextdomain(PACKAGE, CP_DATADIR "/locale");
	
	/* Display startup information */
	cmd_copyright(0, NULL);
	fputs(_("Type \"help\", \"copyright\" or \"license\" for more information.\n"), stdout);

	/* Get prompt texts */
	prompt_no_context = _("[no plug-in context] > ");
	prompt_context = _("[plug-in context %d] > ");
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
			exit(0);
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
		} else if (!strcmp(argv[0], "copyright")) {
			cmd_copyright(argc, argv);
		} else {
			fprintf(stderr, _("ERROR: Unknown command %s\n"), argv[0]);
		}
	}
}
