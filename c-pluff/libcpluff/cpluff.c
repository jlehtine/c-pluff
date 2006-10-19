/*-------------------------------------------------------------------------
 * C-Pluff, a plug-in framework for C
 * Copyright 2006 Johannes Lehtinen
 *-----------------------------------------------------------------------*/

/* Core C-Pluff functions */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>
#include "cpluff.h"
#include "defines.h"


/* ------------------------------------------------------------------------
 * Variables
 * ----------------------------------------------------------------------*/

/** Fatal error handler, or NULL for default */
static cp_error_handler_t fatal_error_handler = NULL;

/** Number of initializations */
static int initialized = 0;

/** Implementation information */
static cp_implementation_info_t implementation_info = {
	CP_RELEASE_VERSION,
	CP_API_VERSION,
	CP_API_REVISION,
	CP_API_AGE,
	CP_HOST,
	CP_THREADS
};

/* ------------------------------------------------------------------------
 * Function definitions
 * ----------------------------------------------------------------------*/

cp_implementation_info_t * CP_API cp_get_implementation_info(void) {
	return &implementation_info;
}

void CP_API cp_init(void) {
	if (!initialized) {
		bindtextdomain(PACKAGE, CP_DATADIR CP_FNAMESEP_STR "locale");
	}
	initialized++;
}

void CP_API cp_destroy(void) {
	assert(initialized > 0);
	initialized--;
}

void CP_API cp_set_fatal_error_handler(cp_error_handler_t error_handler) {
	fatal_error_handler = error_handler;
}

void CP_LOCAL cpi_fatalf(const char *msg, ...) {
	va_list params;
	char fmsg[256];
		
	/* Format message */
	assert(msg != NULL);
	va_start(params, msg);
	vsnprintf(fmsg, sizeof(fmsg), msg, params);
	va_end(params);
	fmsg[sizeof(fmsg)/sizeof(char) - 1] = '\0';

	/* Call error handler or print the error message */
	if (fatal_error_handler != NULL) {
		fatal_error_handler(NULL, fmsg);
	} else {
		fprintf(stderr, _(PACKAGE_NAME ": FATAL ERROR: %s\n"), fmsg);
	}
	
	/* Abort if still alive */
	abort();
}
