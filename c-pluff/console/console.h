/*-------------------------------------------------------------------------
 * C-Pluff, a plug-in framework for C
 * Copyright 2005-2006 Johannes Lehtinen
 *-----------------------------------------------------------------------*/

/* Global declarations */

/* Use some definitions from libcpluff sources */
#include "../libcpluff/defines.h"


/* ------------------------------------------------------------------------
 * Global variables
 * ----------------------------------------------------------------------*/

/* Whether this version is linked with the GNU readline library */
extern const int have_readline;

/* The available commands */
extern const char *commands[];


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
