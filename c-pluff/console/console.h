/*-------------------------------------------------------------------------
 * C-Pluff, a plug-in framework for C
 * Copyright 2006 Johannes Lehtinen
 *-----------------------------------------------------------------------*/

// Global declarations 

#ifdef HAVE_CONFIG_H
#include "../libcpluff/config.h"
#endif
#ifdef HAVE_GETTEXT
#include <libintl.h>
#endif
#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif


/* ------------------------------------------------------------------------
 * Defines
 * ----------------------------------------------------------------------*/

// Gettext defines 
#ifdef HAVE_GETTEXT
#define _(String) gettext(String)
#define gettext_noop(String) String
#define N_(String) gettext_noop(String)
#else
#define _(String) (String)
#define N_(String) String
#define textdomain(Domain)
#define bindtextdomain(Package, Directory)
#endif


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

/** Type for flag information */
typedef struct flag_info_t {
	
	/** The name of the flag */
	char *name;
	
	/** The value of the flag */
	int value;
	
} flag_info_t;


/* ------------------------------------------------------------------------
 * Global variables
 * ----------------------------------------------------------------------*/

/** The available commands */
extern const command_info_t commands[];

/** The available load flags */
extern const flag_info_t load_flags[];


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

#endif //CONSOLE_H_
