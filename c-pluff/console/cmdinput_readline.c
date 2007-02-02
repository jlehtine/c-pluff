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

static cp_plugin_info_t **plugins = NULL;

static char *cp_console_compl_cmdgen(const char *text, int state) {
	static int counter;
	static int textlen;

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
		char *buffer = strdup(commands[counter].name);
		counter++;
		return buffer;
	}
}

static char *cp_console_compl_flagsgen(const char *text, int state) {
	static int counter;
	static int textlen;
	
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
		char *buffer = strdup(load_flags[counter].name);
		counter++;
		return buffer;
	}
}

static char *cp_console_compl_loggen(const char *text, int state) {
	static int counter;
	static int textlen;
	
	if (!state) {
		counter = 0;
		textlen = strlen(text);
	}
	while (log_levels[counter].name != NULL && strncmp(text, log_levels[counter].name, textlen)) {
		counter++;
	}
	if (log_levels[counter].name == NULL) {
		return NULL;
	} else {
		char *buffer = strdup(log_levels[counter].name);
		counter++;
		return buffer;
	}
}

static char *cp_console_compl_plugingen(const char *text, int state) {
	static int counter;
	static int textlen;
	
	if (!state) {
		counter = 0;
		textlen = strlen(text);
		if (plugins != NULL) {
			cp_release_info(context, plugins);
		}
		plugins = cp_get_plugins_info(context, NULL, NULL);
	}
	if (plugins != NULL) {
		while (plugins[counter] != NULL && strncmp(text, plugins[counter]->identifier, textlen)) {
			counter++;
		}
		if (plugins[counter] == NULL) {
			cp_release_info(context, plugins);
			plugins = NULL;
			return NULL;
		} else {
			char *buffer = strdup(plugins[counter]->identifier);
			counter++;
			return buffer;
		}
	} else {
		return NULL;
	}
}

static char **cp_console_completion(const char *text, int start, int end) {
	int cs, ce;
	char **matches = NULL;

	// Search for start and end of command	
	for (cs = 0; cs < start && isspace(rl_line_buffer[cs]); cs++);
	for (ce = cs; ce <= start && !isspace(rl_line_buffer[ce]); ce++);
	
	// If no command entered yet, use command completion
	if (ce >= start) {
		matches = rl_completion_matches(text, cp_console_compl_cmdgen);
		rl_attempted_completion_over = 1;
	}
	
	// Otherwise check if known command and complete accordingly
	else {
		int j = 0;
		while (commands[j].name != NULL
				&& strncmp(rl_line_buffer + cs, commands[j].name, ce - cs)) {
			j++;
		}
		if (commands[j].name != NULL) {
			switch(commands[j].arg_completion) {
				case CPC_COMPL_FILE:
					break;
				case CPC_COMPL_FLAG:
					matches = rl_completion_matches(text, cp_console_compl_flagsgen);
					rl_attempted_completion_over = 1;
					break;
				case CPC_COMPL_LOG_LEVEL:
					matches = rl_completion_matches(text, cp_console_compl_loggen);
					rl_attempted_completion_over = 1;
					break;
				case CPC_COMPL_PLUGIN:
					matches = rl_completion_matches(text, cp_console_compl_plugingen);
					rl_attempted_completion_over = 1;
					break;
				default:
					rl_attempted_completion_over = 1;
					break;
			}
		} else {
			rl_attempted_completion_over = 1;
		}
	}
	return matches;
}

CP_HIDDEN void cmdline_init(void) {
	rl_readline_name = PACKAGE_NAME;
	rl_attempted_completion_function = cp_console_completion;
}

CP_HIDDEN char *cmdline_input(const char *prompt) {
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

CP_HIDDEN void cmdline_destroy(void) {
	if (plugins != NULL) {
		cp_release_info(context, plugins);
		plugins = NULL;
	}
}
