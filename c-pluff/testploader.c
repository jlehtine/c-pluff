/*-------------------------------------------------------------------------
 * C-Pluff, a plug-in framework for C
 * Copyright 2005-2006 Johannes Lehtinen
 *-----------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "cpluff.h"

static const char *strnull(const char *str) {
	return (str != NULL ? str : "<NULL>");
}

static void print_indent(int indentation) {
	while (indentation-- > 0) {
		putchar(' ');
	}
}

static void print_configuration(int indentation, const cp_cfg_element_t *conf) {
	int i;
	
	print_indent(indentation);
	printf("name: %s\n", strnull(conf->name));
	if (conf->num_atts > 0) {
		print_indent(indentation);
		printf("attributes:\n");
		for (i = 0; i < conf->num_atts; i++) {
			print_indent(indentation + 2);
			printf("%s=%s\n", strnull(conf->atts[i*2]), strnull(conf->atts[i*2 + 1]));
		}
	}
	print_indent(indentation);
	printf("value: %s\n", strnull(conf->value));
	if (conf->num_children > 0) {
		print_indent(indentation);
		printf("children:\n");
		for (i = 0; i < conf->num_children; i++) {
			print_indent(indentation + 2);
			printf("%d:\n", conf->children[i].index + 1);
			print_configuration(indentation + 4, conf->children + i);
		}
	}
}

static void error_handler(cp_context_t *context, const char *msg) {
	fputs(msg, stderr);
	fputc('\n', stderr);
}

int main(int argc, char *argv[]) {
	cp_context_t *context;
	int i;
	
	cp_init();
	if ((context = cp_create_context(error_handler, NULL)) == NULL) {
		exit(1);
	}
	if (argc > 1) {
		cp_plugin_t *plugin;
		
		printf("Loading plug-in from %s.\n", argv[1]);
		if ((plugin = cp_load_plugin(context, argv[1], NULL)) != NULL) {
			printf("  name: %s\n", strnull(plugin->name));
			printf("  identifier: %s\n", strnull(plugin->identifier));
			printf("  version: %s\n", strnull(plugin->version));
			printf("  provider name: %s\n", strnull(plugin->provider_name));
			printf("  path: %s\n", strnull(plugin->path));
			printf("  imports:\n");
			for (i = 0; i < plugin->num_imports; i++) {
				printf("    %d:\n", i+1);
				printf("      plugin id: %s\n", strnull(plugin->imports[i].plugin_id));
				printf("      version: %s\n", strnull(plugin->imports[i].version));
				printf("      match: %d\n", (int) (plugin->imports[i].match));
				printf("      optional: %d\n", plugin->imports[i].optional);
			}
			printf("  runtime:\n");
			printf("    library: %s\n", strnull(plugin->lib_path));
			printf("    start function: %s\n", strnull(plugin->start_func_name));
			printf("    stop function: %s\n", strnull(plugin->stop_func_name));
			printf("  extension points:\n");
			for (i = 0; i < plugin->num_ext_points; i++) {
				printf("    %d:\n", i+1);
				printf("      name: %s\n", strnull(plugin->ext_points[i].name));
				printf("      local identifier: %s\n", strnull(plugin->ext_points[i].local_id));
				printf("      global identifier: %s\n", strnull(plugin->ext_points[i].global_id));
				printf("      schema path: %s\n", strnull(plugin->ext_points[i].schema_path));
			}
			printf("  extensions:\n");
			for (i = 0; i < plugin->num_extensions; i++) {
				printf("    %d:\n", i+1);
				printf("      name: %s\n", strnull(plugin->extensions[i].name));
				printf("      local identifier: %s\n", strnull(plugin->extensions[i].local_id));
				printf("      global identifier: %s\n", strnull(plugin->extensions[i].global_id));
				printf("      extension point identifier: %s\n", strnull(plugin->extensions[i].ext_point_id));
				if (plugin->extensions[i].configuration != NULL) {
					printf("      configuration:\n");
					print_configuration(8, plugin->extensions[i].configuration);
				}
			}
			cp_release_plugin(plugin);
		}
	}
	cp_destroy_context(context);
	return 0;
}
