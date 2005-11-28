/*-------------------------------------------------------------------------
 * C-Pluff, a plug-in framework for C
 * Copyright 2005 Johannes Lehtinen
 *-----------------------------------------------------------------------*/

#include "cpluff.h"

int main(int argc, char *argv[]) {
	if (cp_init() == CP_OK) {
		cp_destroy();
	}
	return 0;
}
