#include <stdio.h>
#include <stdlib.h>
#include "plugins-source/callbackcounter/callbackcounter.h"
#include "test.h"

void plugincallbacks(void) {
	cp_context_t *ctx;
	cp_status_t status;
	cp_plugin_info_t *plugin;
	int errors;
	cbc_counters_t *counters;
	
	ctx = init_context(CP_LOG_ERROR, &errors);
	check((plugin = cp_load_plugin_descriptor(ctx, "tmp/install/plugins/callbackcounter", &status)) != NULL && status == CP_OK);
	check(cp_install_plugin(ctx, plugin) == CP_OK);
	cp_release_info(ctx, plugin);
	
	// Start plug-in implicitly by resolving a symbol
	check((counters = cp_resolve_symbol(ctx, "callbackcounter", "cbc_counters", &status)) != NULL && status == CP_OK);
	check(counters->create == 1);
	check(counters->start == 1);
	check(counters->logger == 0);
	check(counters->listener == 1);
	check(counters->run == 0);
	check(counters->stop == 0);
	check(counters->destroy == 0);

	// Cause warning
	check(cp_start_plugin(ctx, "nonexisting") == CP_ERR_UNKNOWN);
	check(counters->create == 1);
	check(counters->start == 1);
	check(counters->logger == 1);
	check(counters->listener == 1);
	check(counters->run == 0);
	check(counters->stop == 0);
	check(counters->destroy == 0);

	// Run run function once
	check(cp_run_plugins_step(ctx));
	check(counters->create == 1);
	check(counters->start == 1);
	check(counters->logger == 1);
	check(counters->listener == 1);
	check(counters->run == 1);
	check(counters->stop == 0);
	check(counters->destroy == 0);

	// Run run function until no more work to be done (run = 3)
	cp_run_plugins(ctx);
	check(counters->create == 1);
	check(counters->start == 1);
	check(counters->logger == 1);
	check(counters->listener == 1);
	check(counters->run == 3);
	check(counters->stop == 0);
	check(counters->destroy == 0);

	/*
	 * Normally symbols must not be accessed after they have been released.
	 * We still access counters here because we know that the plug-in
	 * implementation does not free the counter data.
	 */
	cp_release_symbol(ctx, counters);

	// Stop plug-in
	check(cp_stop_plugin(ctx, "callbackcounter") == CP_OK);
	check(counters->create == 1);
	check(counters->start == 1);
	check(counters->logger == 1);
	check(counters->listener == 2);
	check(counters->run == 3);
	check(counters->stop == 1);
	// for now 1 but might be 0 in future (delay destroy)
	check(counters->destroy == 0 || counters->destroy == 1);
	
	// Uninstall plugin
	check(cp_uninstall_plugin(ctx, "callbackcounter") == CP_OK);
	check(counters->create == 1);
	check(counters->start == 1);
	check(counters->logger == 1);
	check(counters->listener == 2);
	check(counters->run == 3);
	check(counters->stop == 1);
	check(counters->destroy == 1);

	cp_destroy();
	check(errors == 0);
	
	/* Free the counter data that was intentionally leaked by the plug-in */
	free(counters);
}
