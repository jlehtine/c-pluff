#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "test.h"
#include "../libcpluff/internal.h"

static const char *argv0;

CP_HIDDEN void fail(const char *func, const char *file, int line, const char *msg) {
	fprintf(stderr, "%s: %s:%d: %s: %s\n", argv0, file, line, func, msg);
	exit(1);
}

static void full_logger(cp_log_severity_t severity, const char *msg, const char *apid, void *user_data) {
	const char *sevstr;
	switch (severity) {
		case CP_LOG_DEBUG:
			sevstr = "DEBUG";
			break;
		case CP_LOG_INFO:
			sevstr = "INFO";
			break;
		case CP_LOG_WARNING:
			sevstr = "WARNING";
			break;
		case CP_LOG_ERROR:
			sevstr = "ERROR";
			break;
		default:
			check((sevstr = "UNKNOWN", 0));
			break;
	}
	if (apid != NULL) {
		fprintf(stderr, "testsuite: %s: [%s] %s\n", sevstr, apid, msg);
	} else {
		fprintf(stderr, "testsuite: %s: [testsuite] %s\n", sevstr, msg);
	}
	if (severity >= CP_LOG_ERROR && user_data != NULL) {
		(*((int *) user_data))++;		
	}
}

static void counting_logger(cp_log_severity_t severity, const char *msg, const char *apid, void *user_data) {
	(*((int *) user_data))++;
}

CP_HIDDEN cp_context_t *init_context(cp_log_severity_t min_disp_sev, int *error_counter) {
	cp_context_t *ctx;
	cp_status_t status;
	
	check(cp_init() == CP_OK);
	check((ctx = cp_create_context(&status)) != NULL && status == CP_OK);
	if (error_counter != NULL) {
		*error_counter = 0;
	}
	if (error_counter != NULL || min_disp_sev <= CP_LOG_ERROR) {
		if (min_disp_sev <= CP_LOG_ERROR) {
			check(cp_register_logger(ctx, full_logger, error_counter, min_disp_sev) == CP_OK);
		} else {
			check(cp_register_logger(ctx, counting_logger, error_counter, CP_LOG_ERROR) == CP_OK);
		}
	}
	return ctx;
}

CP_HIDDEN const char *plugindir(const char *plugin) {
	static char *buffer = NULL;
	const char *srcdir;
	
	if (buffer != NULL) {
		free(buffer);
		buffer = NULL;
	}
	if ((srcdir = getenv("srcdir")) == NULL) {
		srcdir=".";
	}
	if ((buffer = malloc((strlen(srcdir) + strlen("/plugins/") + strlen(plugin) + 1) * sizeof(char))) == NULL) {
		fputs("testsuite: ERROR: Insufficient memory.\n", stderr);
		exit(2);
	}
	strcpy(buffer, srcdir);
	strcat(buffer, CP_FNAMESEP_STR "plugins" CP_FNAMESEP_STR);
	strcat(buffer, plugin);
	return buffer;
}

CP_HIDDEN const char *pcollectiondir(const char *collection) {
	static char *buffer = NULL;
	const char *srcdir;
	
	if (buffer != NULL) {
		free(buffer);
		buffer = NULL;
	}
	if ((srcdir = getenv("srcdir")) == NULL) {
		srcdir=".";
	}
	if ((buffer = malloc((strlen(srcdir) + strlen("/pcollections/") + strlen(collection) + 1) * sizeof(char))) == NULL) {
		fputs("testsuite: ERROR: Insufficient memory.\n", stderr);
		exit(2);
	}
	strcpy(buffer, srcdir);
	strcat(buffer, CP_FNAMESEP_STR "pcollections" CP_FNAMESEP_STR);
	strcat(buffer, collection);
	return buffer;
}

int main(int argc, char *argv[]) {
	DLHANDLE dh;
	void *ptr;

	// Check arguments
	if (argc != 2) {
		fputs("testsuite: ERROR: Usage: testsuite <test>\n", stderr);
		exit(2);
	}
	if ((argv0 = argv[0]) == NULL) {
		argv0 = "testsuite";
	}

	// Find the test
	if ((dh = DLOPEN(NULL)) == NULL) {
		fputs("testsuite: ERROR: Could not open the testsuite binary for symbols.\n", stderr);
		exit(2);
	}
	if ((ptr = DLSYM(dh, argv[1])) == NULL) {
		fprintf(stderr, "testsuite: ERROR: Could not resolve symbol %s.\n", argv[1]);
		exit(2);
	}
	
	// Execute the test
	((void (*)(void)) ptr)();
	
	// Successfully completed
	exit(0);
}
