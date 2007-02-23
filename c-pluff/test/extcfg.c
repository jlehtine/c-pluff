#include <string.h>
#include "test.h"

void extcfgutils(void) {
	cp_context_t *ctx;
	cp_plugin_info_t *plugin;
	cp_extension_t *ext;
	cp_cfg_element_t *ce, *cebase;
	const char *str;
	int errors;
	cp_status_t status;
	int i;
	
	ctx = init_context(CP_LOG_ERROR, &errors);
	check((plugin = cp_load_plugin_descriptor(ctx, plugindir("maximal"), &status)) != NULL && status == CP_OK);
	for (i = 0, ext = NULL; ext == NULL && i < plugin->num_extensions; i++) {
		cp_extension_t *e = plugin->extensions + i;
		if (e->identifier != NULL && !strcmp(e->local_id, "ext1")) {
			ext = e;
		}
	}
	check(ext != NULL);
	
	// Look up using forward path
	check((ce = cp_lookup_cfg_element(ext->configuration, "structure/parameter")) != NULL && ce->value != NULL && strcmp(ce->value, "parameter") == 0);
	check((ce = cebase = cp_lookup_cfg_element(ext->configuration, "structure/deeper/struct/is")) != NULL && ce->value != NULL && strcmp(ce->value, "here") == 0);
	check((str = cp_lookup_cfg_value(ext->configuration, "structure/parameter")) != NULL && strcmp(str, "parameter") == 0);
	check((str = cp_lookup_cfg_value(ext->configuration, "@name")) != NULL && strcmp(str, "Extension 1") == 0);
	
	// Look up using reverse path
	check((ce = cp_lookup_cfg_element(cebase, "../../../parameter/../deeper")) != NULL && strcmp(ce->name, "deeper") == 0);
	check((str = cp_lookup_cfg_value(cebase, "../../../../@name")) != NULL && strcmp(str, "Extension 1") == 0);
	
	// Look up nonexisting components
	check(cp_lookup_cfg_element(ext->configuration, "non/existing") == NULL);
	check(cp_lookup_cfg_element(ext->configuration, "structure/../..") == NULL);
	check(cp_lookup_cfg_value(ext->configuration, "non/existing") == NULL);
	check(cp_lookup_cfg_value(ext->configuration, "structure/../..") == NULL);
	check(cp_lookup_cfg_value(ext->configuration, "structure@nonexisting") == NULL);

	cp_release_info(ctx, plugin);
	cp_destroy_context(ctx);
	check(errors == 0); 
}
