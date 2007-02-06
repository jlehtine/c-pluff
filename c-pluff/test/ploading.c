#include <stdio.h>
#include "test.h"

void loadonlymaximal(void) {
	cp_context_t *ctx;
	cp_plugin_info_t *plugin;
	cp_status_t status;
	int errors;
	
	ctx = init_context(CP_LOG_ERROR, &errors);
	check((plugin = cp_load_plugin_descriptor(ctx, plugindir("maximal"), &status)) != NULL && status == CP_OK);
	cp_release_info(ctx, plugin);
	cp_destroy();
	check(errors == 0);
}
