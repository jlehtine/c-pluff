#include <stdio.h>
#include <stdlib.h>
#include "test.h"

void fatalerrordefault(void) {
	cp_start_plugin(NULL, NULL);
}

static void error_handler(const char *msg) {
	exit(0);
}

void fatalerrorhandled(void) {
	cp_set_fatal_error_handler(error_handler);
	cp_start_plugin(NULL, NULL);
	exit(1);
}

void fatalerrorreset(void) {
	cp_set_fatal_error_handler(error_handler);
	cp_set_fatal_error_handler(NULL);
	cp_start_plugin(NULL, NULL);
}
