#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "test.h"

void errorlogger(void) {
	cp_context_t *ctx;
	cp_status_t status;
	int errors;
	
	ctx = init_context(CP_LOG_ERROR + 1, &errors);
	check(cp_load_plugin_descriptor(ctx, "nonexisting", &status) == NULL && status != CP_OK);
	cp_destroy();
	check(errors > 0);
}

struct log_count_t {
	cp_log_severity_t max_severity;
	int count_max;
	int count_above_max;
};

static void counting_logger(cp_log_severity_t severity, const char *msg, const char *apid, void *user_data) {
	struct log_count_t *lc = user_data;
	
	if (severity <= lc->max_severity) {
		lc->count_max++;
	} else {
		lc->count_above_max++;
	}
}

void warninglogger(void) {
	cp_context_t *ctx;
	struct log_count_t lc = { CP_LOG_WARNING, 0, 0 };
	
	ctx = init_context(CP_LOG_ERROR, NULL);
	check(cp_register_logger(ctx, counting_logger, &lc, CP_LOG_WARNING) == CP_OK);
	check(cp_start_plugin(ctx, "nonexisting") == CP_ERR_UNKNOWN);
	cp_destroy();
	check(lc.count_max > 0 && lc.count_above_max == 0);
}

void infologger(void) {
	cp_context_t *ctx;
	cp_plugin_info_t *plugin;
	cp_status_t status;
	struct log_count_t lc = { CP_LOG_INFO, 0, 0 };

	ctx = init_context(CP_LOG_WARNING, NULL);
	check(cp_register_logger(ctx, counting_logger, &lc, CP_LOG_INFO) == CP_OK);
	check((plugin = cp_load_plugin_descriptor(ctx, plugindir("minimal"), &status)) != NULL && status == CP_OK);
	check(cp_install_plugin(ctx, plugin) == CP_OK);
	cp_release_info(ctx, plugin);
	cp_destroy();
	check(lc.count_max > 0 && lc.count_above_max == 0);
}

void debuglogger(void) {
	cp_context_t *ctx;
	struct log_count_t lc = { CP_LOG_DEBUG, 0, 0 };
	
	ctx = init_context(CP_LOG_INFO, NULL);
	check(cp_register_logger(ctx, counting_logger, &lc, CP_LOG_DEBUG) == CP_OK);
	cp_destroy();
	check(lc.count_max > 0 && lc.count_above_max == 0);
}

static void increment_logger(cp_log_severity_t severity, const char *msg, const char *apid, void *user_data) {
	(*((int *) user_data))++;
}

void twologgers(void) {
	cp_context_t *ctx;
	cp_plugin_info_t *plugin;
	cp_status_t status;
	struct log_count_t lc = { CP_LOG_DEBUG, 0, 0 };
	int count = 0;
	int errors;
	
	ctx = init_context(CP_LOG_ERROR, &errors);
	check(cp_register_logger(ctx, counting_logger, &lc, CP_LOG_DEBUG) == CP_OK);
	check(count == 0 && lc.count_max > 0 && lc.count_above_max == 0);
	check(cp_register_logger(ctx, increment_logger, &count, CP_LOG_INFO) == CP_OK);
	check(count == 0 && lc.count_max > 0 && lc.count_above_max == 0);
	check((plugin = cp_load_plugin_descriptor(ctx, plugindir("minimal"), &status)) != NULL && status == CP_OK);
	check(cp_install_plugin(ctx, plugin) == CP_OK);
	cp_release_info(ctx, plugin);
	check(count > 0 && lc.count_max > 0 && lc.count_above_max > 0);
	cp_destroy();
	check(errors == 0);
}

void unreglogger(void) {
	cp_context_t *ctx;
	cp_plugin_info_t *plugin;
	cp_status_t status;
	struct log_count_t lc = { CP_LOG_DEBUG, 0, 0 };
	int count = 0;
	int errors;
	
	ctx = init_context(CP_LOG_ERROR, &errors);
	check(cp_register_logger(ctx, counting_logger, &lc, CP_LOG_DEBUG) == CP_OK);
	check(count == 0 && lc.count_max > 0 && lc.count_above_max == 0);
	check(cp_register_logger(ctx, increment_logger, &count, CP_LOG_INFO) == CP_OK);
	check(count == 0 && lc.count_max > 0 && lc.count_above_max == 0);
	cp_unregister_logger(ctx, counting_logger);
	check((plugin = cp_load_plugin_descriptor(ctx, plugindir("minimal"), &status)) != NULL && status == CP_OK);
	check(cp_install_plugin(ctx, plugin) == CP_OK);
	cp_release_info(ctx, plugin);
	check(count > 0 && lc.count_max > 0 && lc.count_above_max == 0);
	cp_destroy();
	check(errors == 0);
}

void updatelogger(void) {
	cp_context_t *ctx;
	cp_plugin_info_t *plugin;
	cp_status_t status;
	struct log_count_t lc = { CP_LOG_DEBUG, 0, 0 };
	struct log_count_t lc2 = { CP_LOG_INFO, 0, 0 };
	int count = 0;
	int errors;
	
	ctx = init_context(CP_LOG_ERROR, &errors);
	check(cp_register_logger(ctx, counting_logger, &lc, CP_LOG_DEBUG) == CP_OK);
	check(count == 0 && lc.count_max > 0 && lc.count_above_max == 0);
	check(cp_register_logger(ctx, increment_logger, &count, CP_LOG_INFO) == CP_OK);
	check(count == 0 && lc.count_max > 0 && lc.count_above_max == 0);
	check(cp_register_logger(ctx, counting_logger, &lc2, CP_LOG_DEBUG) == CP_OK);
	check((plugin = cp_load_plugin_descriptor(ctx, plugindir("minimal"), &status)) != NULL && status == CP_OK);
	check(cp_install_plugin(ctx, plugin) == CP_OK);
	cp_release_info(ctx, plugin);
	check(count > 0 && lc.count_max > 0 && lc.count_above_max == 0);
	cp_destroy();
	check(errors == 0);
}

struct log_info_t {
	cp_log_severity_t severity;
	char *msg;
	char *apid;
};

static void store_logger(cp_log_severity_t severity, const char *msg, const char *apid, void *user_data) {
	struct log_info_t *li = user_data;
	
	// Free previous data
	if (li->msg != NULL) {
		free(li->msg);
		li->msg = NULL;
	}
	if (li->apid != NULL) {
		free(li->apid);
		li->apid = NULL;
	}
	
	// Copy information
	li->severity = severity;
	if (msg != NULL) {
		check((li->msg = strdup(msg)) != NULL);
	}
	if (apid != NULL) {
		check((li->apid = strdup(apid)) != NULL);
	}
}

static void logmsg_sev(cp_context_t *ctx, cp_log_severity_t severity, const char *msg) {
	struct log_info_t li = { -1, NULL, NULL };

	check(cp_register_logger(ctx, store_logger, &li, CP_LOG_DEBUG) == CP_OK);
	cp_log(ctx, severity, msg);
	check(li.severity == severity);
	check(li.msg != NULL && !strcmp(li.msg, msg));
	check(li.apid == NULL);
	free(li.msg);
	li.msg = NULL;
	cp_unregister_logger(ctx, store_logger);
}

void logmsg(void) {
	cp_context_t *ctx;
	
	ctx = init_context(CP_LOG_ERROR + 1, NULL);
	logmsg_sev(ctx, CP_LOG_DEBUG, "debug");
	logmsg_sev(ctx, CP_LOG_INFO, "info");
	logmsg_sev(ctx, CP_LOG_WARNING, "warning");
	logmsg_sev(ctx, CP_LOG_ERROR, "error");
	cp_destroy();
}

static void islogged_sev(cp_context_t *ctx, cp_log_severity_t severity) {
	int count = 0;
	
	check(!cp_is_logged(ctx, severity));
	check(cp_register_logger(ctx, increment_logger, &count, severity) == CP_OK);
	check(cp_is_logged(ctx, CP_LOG_ERROR));
	check(cp_is_logged(ctx, severity));
	switch (severity) {
		case CP_LOG_DEBUG:
			break;
		case CP_LOG_INFO:
			check(!cp_is_logged(ctx, CP_LOG_DEBUG));
			break;
		case CP_LOG_WARNING:
			check(!cp_is_logged(ctx, CP_LOG_INFO));
			break;
		case CP_LOG_ERROR:
			check(!cp_is_logged(ctx, CP_LOG_WARNING));
			break;
	}
	cp_unregister_logger(ctx, increment_logger);
}

void islogged(void) {
	cp_context_t *ctx;
	
	ctx = init_context(CP_LOG_ERROR + 1, NULL);
	islogged_sev(ctx, CP_LOG_DEBUG);
	islogged_sev(ctx, CP_LOG_INFO);
	islogged_sev(ctx, CP_LOG_WARNING);
	islogged_sev(ctx, CP_LOG_ERROR);
	cp_destroy();
}
