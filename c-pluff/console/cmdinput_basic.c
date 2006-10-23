/*-------------------------------------------------------------------------
 * C-Pluff, a plug-in framework for C
 * Copyright 2006 Johannes Lehtinen
 *-----------------------------------------------------------------------*/

// Basic command line input functionality 

#include <stdio.h>
#include <string.h>
#include "console.h"

#define CMDLINE_SIZE 256

void cmdline_init(void) {}

char *cmdline_input(const char *prompt) {
	static char cmdline[CMDLINE_SIZE];
	int i, success = 0;
	
	do {
		fputs(prompt, stdout);
		if (fgets(cmdline, CMDLINE_SIZE, stdin) == NULL) {
			return NULL;
		}
		if (strlen(cmdline) == CMDLINE_SIZE - 1
			&& cmdline[CMDLINE_SIZE - 2] != '\n') {
			do {
				if (fgets(cmdline, CMDLINE_SIZE, stdin) == NULL) {
					return NULL;
				}
			} while (strlen(cmdline) == CMDLINE_SIZE - 1
				&& cmdline[CMDLINE_SIZE - 2] != '\n');
			fputs(_("ERROR: Command line too long.\n"), stderr);
		} else {
			success = 1;
		}
	} while (!success);
	i = strlen(cmdline);
	if (i > 0 && cmdline[i - 1] == '\n') {
		cmdline[i - 1] = '\0';
	}
	return cmdline;
}
