/*-------------------------------------------------------------------------
 * C-Pluff, a plug-in framework for C
 * Copyright 2005-2006 Johannes Lehtinen
 *-----------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "cpluff.h"

static void error_handler(const char *msg);

int main(int argc, char *argv[]) {
	cp_id_t id;
	int i;
	
	if (cp_init(error_handler) != CP_OK) {
		exit(1);
	}
	if (argc > 1) {
		printf("Loading plug-in from %s.\n", argv[1]);
		if (cp_load_plugin(argv[1], &id) == CP_OK) {
			const cp_plugin_t *plugin;
			
			printf("Loaded plug-in %s:\n", id);
			if ((plugin = cp_get_plugin(id, NULL)) == NULL) {
				fprintf(stderr, "Failed to get plug-in %s.\n", id);
				exit(1);
			}
			printf("  name: %s\n", plugin->name);
			printf("  identifier: %s\n", plugin->identifier);
			printf("  version: %s\n", plugin->version);
			printf("  provider name: %s\n", plugin->provider_name);
			printf("  path: %s\n", plugin->path);
			printf("  imports:\n");
			for (i = 0; i < plugin->num_imports; i++) {
				printf("    %d:\n", i+1);
				printf("      plugin id: %s\n", plugin->imports[i].plugin_id);
				printf("      version: %s\n", plugin->imports[i].version);
				printf("      match: %d\n", (int) (plugin->imports[i].match));
				printf("      optional: %d\n", plugin->imports[i].optional);
			}
			printf("  runtime:\n");
			printf("    library: %s\n", plugin->lib_path);
			printf("    start function: %s\n", plugin->start_func_name);
			printf("    stop function: %s\n", plugin->stop_func_name);
			printf("  extension points:\n");
			for (i = 0; i < plugin->num_ext_points; i++) {
				printf("    %d:\n", i+1);
				printf("      name: %s\n", plugin->ext_points[i].name);
				printf("      simple identifier: %s\n", plugin->ext_points[i].simple_id);
				printf("      unique identifier: %s\n", plugin->ext_points[i].extpt_id);
			}
			cp_release_plugin(plugin);
		}
	}
	cp_destroy();
	return 0;
}

static void error_handler(const char *msg) {
	fputs(msg, stderr);
	fputc('\n', stderr);
}
