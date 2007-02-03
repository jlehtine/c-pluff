#include <stdio.h>
#include <string.h>
#include "test.h"
#include <cpluff.h>

void getversion(void) {
	check(cp_get_version() != NULL);
	check(!strcmp(cp_get_version(), CP_VERSION));
}

void gethosttype(void) {
	check(cp_get_host_type() != NULL);
	check(!strcmp(cp_get_host_type(), CP_HOST));
}
