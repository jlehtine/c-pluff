/*-------------------------------------------------------------------------
 * C-Pluff, a plug-in framework for C
 * Copyright 2006 Johannes Lehtinen
 *-----------------------------------------------------------------------*/

/* Global declarations */

/* Use some definitions from libcpluff sources */
#include "../libcpluff/defines.h"


/* ------------------------------------------------------------------------
 * Data types
 * ----------------------------------------------------------------------*/

/** Type for command implementations */
typedef void (*command_func_t)(int argc, char *argv[]);

/** Type for command information */
typedef struct command_info_t {
	
	/** The name of the command */
	char *name;
	
	/** The description for the command */
	char *description;
	
	/** The command implementation */
	command_func_t implementation;
	
} command_info_t;


/* ------------------------------------------------------------------------
 * Global variables
 * ----------------------------------------------------------------------*/

/* Whether this version is linked with the GNU readline library */
extern const int have_readline;

/* The available commands */
extern const command_info_t commands[];


/* ------------------------------------------------------------------------
 * Function declarations
 * ----------------------------------------------------------------------*/

/**
 * Initializes command line reading. Must be called once to initialize
 * everything before using cmdline_input.
 */
void cmdline_init(void);

/**
 * Returns a command line entered by the user. Uses the specified prompt.
 * The returned command line is valid and it may be modified until the
 * next call to this function.
 * 
 * @param prompt the prompt to be used
 * @return the command line entered by the user
 */
char *cmdline_input(const char *prompt);

#ifndef CONSOLE_H_
#define CONSOLE_H_

#endif /*CONSOLE_H_*/
