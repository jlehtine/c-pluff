/*-------------------------------------------------------------------------
 * C-Pluff, a plug-in framework for C
 * Copyright 2005-2006 Johannes Lehtinen
 *-----------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include "cpluff.h"

static void error_handler(const char *msg);

int main(int argc, char *argv[]) {
	cp_id_t id;
	
	if (cp_init(error_handler) != CP_OK) {
		exit(1);
	}
	if (argc > 1) {
		printf("Loading plug-in from %s.\n", argv[1]);
		if (cp_load_plugin(argv[1], &id) == CP_OK) {
			printf("Loaded plug-in %s.\n", id);
		}
	}
	cp_destroy();
	return 0;
}

static void error_handler(const char *msg) {
	fputs(msg, stderr);
	fputc('\n', stderr);
}
