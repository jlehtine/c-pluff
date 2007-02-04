#include <stdio.h>
#include "plugins-source/callbackcounter/callbackcounter.h"
#include "test.h"

void plugincallbacks(void) {
	cp_context_t *ctx;
	cp_status_t status;
	cp_plugin_info_t *plugin;
	int errors;
	cbc_counters_t *counters;
	
	ctx = init_context(CP_LOG_DEBUG, &errors);
	check((plugin = cp_load_plugin_descriptor(ctx, "tmp-install/plugins/callbackcounter", &status)) != NULL && status == CP_OK);
	check(cp_install_plugin(ctx, plugin) == CP_OK);
	cp_release_info(ctx, plugin);
	check((counters = cp_resolve_symbol(ctx, "callbackcounter", "cbc_counters", &status)) != NULL && status == CP_OK);
	check(counters->create == 1);
	check(counters->start == 1);
	check(counters->stop == 0);
	check(counters->destroy == 0);
	cp_release_symbol(ctx, counters);
	
	/*
	 * Normally symbols must not be accessed after they have been released.
	 * We can still access counters because we know that the plug-in
	 * implementation does not free the counter data.
	 */

	check(cp_stop_plugin(ctx, "callbackcounter") == CP_OK);
	check(counters->create == 1);
	check(counters->start == 1);
	check(counters->stop == 1);
	// for now 1 but might be 0 in future (delay destroy)
	check(counters->destroy >= 0 && counters->destroy <= 1);
	check(cp_uninstall_plugin(ctx, "callbackcounter") == CP_OK);
	check(counters->create == 1);
	check(counters->start == 1);
	check(counters->stop == 1);
	check(counters->destroy == 1);

	cp_destroy();
	check(errors == 0);
	
	/* Free the counter data that was leaked by the plug-in */
	free(counters);
}
