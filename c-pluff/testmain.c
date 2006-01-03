/*-------------------------------------------------------------------------
 * C-Pluff, a plug-in framework for C
 * Copyright 2005-2006 Johannes Lehtinen
 *-----------------------------------------------------------------------*/

#include <stdio.h>
#include "cpluff.h"

static void error_handler(const char *msg);

int main(int argc, char *argv[]) {
	if (cp_init(error_handler) == CP_OK) {
		cp_destroy();
	}
	return 0;
}

static void error_handler(const char *msg) {
	fputs(msg, stderr);
	fputc('\n', stderr);
}
