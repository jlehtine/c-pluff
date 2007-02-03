#include <stdio.h>
#include "test.h"

void nocollections(void) {
	cp_context_t *ctx;
	cp_plugin_info_t **plugins;
	cp_status_t status;
	int errors;
	int i;
	
	ctx = init_context(CP_LOG_ERROR, &errors);
	check(cp_scan_plugins(ctx, 0) == CP_OK);
	check((plugins = cp_get_plugins_info(ctx, &status, &i)) != NULL && status == CP_OK && i == 0);
	cp_release_info(ctx, plugins);
	cp_destroy();
	check(errors == 0);
}

void onecollection(void) {
	cp_context_t *ctx;
	int errors;
	
	ctx = init_context(CP_LOG_ERROR, &errors);
	check(cp_register_pcollection(ctx, pcollectiondir("collection1")) == CP_OK);
	check(cp_scan_plugins(ctx, 0) == CP_OK);
	check(cp_get_plugin_state(ctx, "plugin1") == CP_PLUGIN_INSTALLED);
	cp_destroy();
	check(errors == 0);
}

void twocollections(void) {
	cp_context_t *ctx;
	int errors;
	
	ctx = init_context(CP_LOG_ERROR, &errors);
	check(cp_register_pcollection(ctx, pcollectiondir("collection1")) == CP_OK);
	check(cp_register_pcollection(ctx, pcollectiondir("collection2")) == CP_OK);
	check(cp_scan_plugins(ctx, 0) == CP_OK);
	check(cp_get_plugin_state(ctx, "plugin1") == CP_PLUGIN_INSTALLED);
	check(cp_get_plugin_state(ctx, "plugin2a") == CP_PLUGIN_INSTALLED);
	check(cp_get_plugin_state(ctx, "plugin2b") == CP_PLUGIN_INSTALLED);
	cp_destroy();
	check(errors == 0);
}

void unregcollection(void) {
	cp_context_t *ctx;
	int errors;
	
	ctx = init_context(CP_LOG_ERROR, &errors);
	check(cp_register_pcollection(ctx, pcollectiondir("collection1")) == CP_OK);
	check(cp_register_pcollection(ctx, pcollectiondir("collection2")) == CP_OK);
	cp_unregister_pcollection(ctx, pcollectiondir("collection2"));
	check(cp_scan_plugins(ctx, 0) == CP_OK);
	check(cp_get_plugin_state(ctx, "plugin1") == CP_PLUGIN_INSTALLED);
	check(cp_get_plugin_state(ctx, "plugin2a") == CP_PLUGIN_UNINSTALLED);
	check(cp_get_plugin_state(ctx, "plugin2b") == CP_PLUGIN_UNINSTALLED);
	cp_destroy();
	check(errors == 0);
}

void unregcollections(void) {
	cp_context_t *ctx;
	int errors;
	
	ctx = init_context(CP_LOG_ERROR, &errors);
	check(cp_register_pcollection(ctx, pcollectiondir("collection1")) == CP_OK);
	check(cp_register_pcollection(ctx, pcollectiondir("collection2")) == CP_OK);
	cp_unregister_pcollections(ctx);
	check(cp_scan_plugins(ctx, 0) == CP_OK);
	check(cp_get_plugin_state(ctx, "plugin1") == CP_PLUGIN_UNINSTALLED);
	check(cp_get_plugin_state(ctx, "plugin2a") == CP_PLUGIN_UNINSTALLED);
	check(cp_get_plugin_state(ctx, "plugin2b") == CP_PLUGIN_UNINSTALLED);
	cp_destroy();
	check(errors == 0);
}
