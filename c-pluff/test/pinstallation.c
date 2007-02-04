#include <stdio.h>
#include "test.h"

void install(void) {
	cp_context_t *ctx;
	cp_plugin_info_t *plugin;
	cp_status_t status;
	int errors;
	
	ctx = init_context(CP_LOG_ERROR, &errors);
	check(cp_get_plugin_state(ctx, "minimal") == CP_PLUGIN_UNINSTALLED);
	check((plugin = cp_load_plugin_descriptor(ctx, plugindir("minimal"), &status)) != NULL && status == CP_OK);
	check(cp_get_plugin_state(ctx, "minimal") == CP_PLUGIN_UNINSTALLED);
	check(cp_install_plugin(ctx, plugin) == CP_OK);
	cp_release_info(ctx, plugin);
	check(cp_get_plugin_state(ctx, "minimal") == CP_PLUGIN_INSTALLED);	
	cp_destroy();
	check(errors == 0);
}

void installtwo(void) {
	cp_context_t *ctx;
	cp_plugin_info_t *plugin;
	cp_status_t status;
	int errors;
	
	ctx = init_context(CP_LOG_ERROR, &errors);
	check(cp_get_plugin_state(ctx, "minimal") == CP_PLUGIN_UNINSTALLED);
	check((plugin = cp_load_plugin_descriptor(ctx, plugindir("minimal"), &status)) != NULL && status == CP_OK);
	check(cp_get_plugin_state(ctx, "minimal") == CP_PLUGIN_UNINSTALLED);
	check(cp_install_plugin(ctx, plugin) == CP_OK);
	cp_release_info(ctx, plugin);
	check(cp_get_plugin_state(ctx, "minimal") == CP_PLUGIN_INSTALLED);
	check(cp_get_plugin_state(ctx, "maximal") == CP_PLUGIN_UNINSTALLED);
	check((plugin = cp_load_plugin_descriptor(ctx, plugindir("maximal"), &status)) != NULL && status == CP_OK);
	check(cp_get_plugin_state(ctx, "maximal") == CP_PLUGIN_UNINSTALLED);
	check(cp_install_plugin(ctx, plugin) == CP_OK);
	cp_release_info(ctx, plugin);		
	check(cp_get_plugin_state(ctx, "minimal") == CP_PLUGIN_INSTALLED);
	check(cp_get_plugin_state(ctx, "maximal") == CP_PLUGIN_INSTALLED);
	cp_destroy();
	check(errors == 0);	
}

void installconflict(void) {
	cp_context_t *ctx;
	cp_plugin_info_t *plugin;
	cp_status_t status;
	
	ctx = init_context(CP_LOG_ERROR + 1, NULL);
	check(cp_get_plugin_state(ctx, "minimal") == CP_PLUGIN_UNINSTALLED);
	check((plugin = cp_load_plugin_descriptor(ctx, plugindir("minimal"), &status)) != NULL && status == CP_OK);
	check(cp_get_plugin_state(ctx, "minimal") == CP_PLUGIN_UNINSTALLED);
	check(cp_install_plugin(ctx, plugin) == CP_OK);
	cp_release_info(ctx, plugin);
	check(cp_get_plugin_state(ctx, "minimal") == CP_PLUGIN_INSTALLED);	
	check((plugin = cp_load_plugin_descriptor(ctx, plugindir("minimal"), &status)) != NULL && status == CP_OK);
	check(cp_install_plugin(ctx, plugin) == CP_ERR_CONFLICT);
	cp_release_info(ctx, plugin);
	cp_destroy();
}

void uninstall(void) {
	cp_context_t *ctx;
	cp_plugin_info_t *plugin;
	cp_status_t status;
	int errors;
	
	ctx = init_context(CP_LOG_ERROR, &errors);
	check((plugin = cp_load_plugin_descriptor(ctx, plugindir("minimal"), &status)) != NULL && status == CP_OK);
	check(cp_install_plugin(ctx, plugin) == CP_OK);
	cp_release_info(ctx, plugin);
	check(cp_get_plugin_state(ctx, "minimal") == CP_PLUGIN_INSTALLED);
	check(cp_uninstall_plugin(ctx, "minimal") == CP_OK);
	check(cp_get_plugin_state(ctx, "minimal") == CP_PLUGIN_UNINSTALLED);	
	cp_destroy();
	check(errors == 0);	
}
