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
	data->counters->create++;
	
	return data;
}

static int run(void *d) {
	struct runtime_data *data = d;
	
	data->counters->run++;
	return (data->counters->run < 3);
}

static int start(void *d) {
	struct runtime_data *data = d;
	
	data->counters->start++;
	if (cp_define_symbol(data->ctx, "cbc_counters", data->counters) != CP_OK
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
