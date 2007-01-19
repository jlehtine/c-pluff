/*-------------------------------------------------------------------------
 * C-Pluff, a plug-in framework for C
 * Copyright 2006 Johannes Lehtinen
 *-----------------------------------------------------------------------*/

// GNU readline based command line input 

#include "console.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <readline/readline.h>
#include <readline/history.h>

static char *cp_console_compl_flagsgen(const char *text, int state) {
	static int counter;
	static int textlen;
	char *buffer;
	
	if (!state) {
		counter = 0;
		textlen = strlen(text);
	}
	while(load_flags[counter].name != NULL && strncmp(text, load_flags[counter].name, textlen)) {
		counter++;
	}
	if (load_flags[counter].name == NULL) {
		return NULL;
	} else {
		buffer = malloc(sizeof(char) * (strlen(load_flags[counter].name) + 1));
		if (buffer != NULL) {
			strcpy(buffer, load_flags[counter].name);
		}
		counter++;
		return buffer;
	}
}

static char *cp_console_compl_cmdgen(const char *text, int state) {
	static int counter;
	static int textlen;
	char *buffer;

	if (!state) {
		counter = 0;
		textlen = strlen(text);
	}
	while (commands[counter].name != NULL && strncmp(text, commands[counter].name, textlen)) {
		counter++;
	}
	if (commands[counter].name == NULL) {
		return NULL;
	} else {
		buffer = malloc(sizeof(char) * (strlen(commands[counter].name) + 1));
		if (buffer != NULL) {
			strcpy(buffer, commands[counter].name);
		}
		counter++;
		return buffer;
	}
}

static char **cp_console_completion(const char *text, int start, int end) {
	int i;
	char **matches = NULL;
	
	for (i = 0; i < start && isspace(rl_line_buffer[i]); i++);
	if (i >= start) {
		matches = rl_completion_matches(text, cp_console_compl_cmdgen);
		rl_attempted_completion_over = 1;
	} else if (!strncmp(rl_line_buffer + i, "load-plugins", start - i < 12 ? start - i : 12)) {
		matches = rl_completion_matches(text, cp_console_compl_flagsgen);
		rl_attempted_completion_over = 1;
	}
	return matches;
}

void cmdline_init(void) {
	rl_readline_name = PACKAGE_NAME;
	rl_attempted_completion_function = cp_console_completion;
}

char *cmdline_input(const char *prompt) {
	static char *cmdline = NULL;
	
	// Free previously returned command line, if any 
	if (cmdline != NULL) {
		free(cmdline);
		cmdline = NULL;
	}
	
	// Obtain new command line and record it for history 
	cmdline = readline(prompt);
	if (cmdline != NULL && *cmdline != '\0') {
		add_history(cmdline);
	}
	
	return cmdline;
}
