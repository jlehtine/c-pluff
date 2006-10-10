/*-------------------------------------------------------------------------
 * C-Pluff, a plug-in framework for C
 * Copyright 2005-2006 Johannes Lehtinen
 *-----------------------------------------------------------------------*/

/* Global declarations */

/* Use some definitions from libcpluff sources */
#include "../libcpluff/defines.h"

/* ------------------------------------------------------------------------
 * Function declarations
 * ----------------------------------------------------------------------*/

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
