#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <cpluff.h>
#include "callbackcounter.h"

struct runtime_data {
	cp_context_t *ctx;
	cbc_counters_t *counters;
};

static void *create(cp_context_t *ctx) {
	struct runtime_data *data;
	
	if ((data = malloc(sizeof(struct runtime_data))) == NULL) {
		return NULL;
	}
	data->ctx = ctx;
	
	/*
	 * Normally data->counters would be initialized in start function.
	 * We do it already here to be able to record count for the create
	 * function.
	 */
	if ((data->counters = malloc(sizeof(cbc_counters_t))) == NULL) {
		free(data);
		return NULL;
	}
	memset(data->counters, 0, sizeof(cbc_counters_t));
	data->counters->context_arg_0 = NULL;
	data->counters->create++;
	
	return data;
}

static void logger(cp_log_severity_t severity, const char *msg, const char *apid, void *user_data) {
	struct runtime_data *data = user_data;
	
	data->counters->logger++;
}

static void listener(const char *plugin_id, cp_plugin_state_t old_state, cp_plugin_state_t new_state, void *user_data) {
	struct runtime_data *data = user_data;
	
	data->counters->listener++;
}

static int run(void *d) {
	struct runtime_data *data = d;
	
	data->counters->run++;
	return (data->counters->run < 3);
}

static int start(void *d) {
	struct runtime_data *data = d;
	char **argv;
	
	data->counters->start++;
	argv = cp_get_context_args(data->ctx, NULL);
	if (argv != NULL && argv[0] != NULL) {
		if ((data->counters->context_arg_0 = strdup(argv[0])) == NULL) {
			return CP_ERR_RESOURCE;
		}
	}
	if (cp_define_symbol(data->ctx, "cbc_counters", data->counters) != CP_OK
		|| cp_register_logger(data->ctx, logger, data, CP_LOG_WARNING) != CP_OK
		|| cp_register_plistener(data->ctx, listener, data) != CP_OK
		|| cp_run_function(data->ctx, run) != CP_OK) {
		return CP_ERR_RUNTIME;
	} else {
		return CP_OK;
	}	
}

static void stop(void *d) {
	struct runtime_data *data = d;

	data->counters->stop++;

	/* 
	 * Normally data->counters would be freed here. However, we do not free
	 * it so that the test program can read counters after plug-in stops.
	 */
}

static void destroy(void *d) {
	struct runtime_data *data = d;

	data->counters->destroy++;	
	data->counters = NULL;
	free(data);
}

CP_EXPORT cp_plugin_runtime_t cbc_runtime = {
	create,
	start,
	stop,
	destroy
};
