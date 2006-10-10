/*-------------------------------------------------------------------------
 * C-Pluff, a plug-in framework for C
 * Copyright 2005-2006 Johannes Lehtinen
 *-----------------------------------------------------------------------*/

/* GNU readline based command line input */

#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "console.h"

char *cmdline_input(const char *prompt) {
	static char *cmdline = NULL;
	
	/* Free previously returned command line, if any */
	if (cmdline != NULL) {
		free(cmdline);
		cmdline = NULL;
	}
	
	/* Obtain new command line and record it for history */
	cmdline = readline(prompt);
	if (cmdline != NULL && *cmdline != '\0') {
		add_history(cmdline);
	}
	
	return cmdline;
}
