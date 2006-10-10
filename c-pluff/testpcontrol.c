/*-------------------------------------------------------------------------
 * C-Pluff, a plug-in framework for C
 * Copyright 2006 Johannes Lehtinen
 *-----------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "libcpluff/cpluff.h"

static const char *commands[] = {
	"exit",
	"add-plugin-dir",
	"remove-plugin-dir",
	"load-plugin",
	"load-plugins",
	"list-plugins",
	"start-plugin",
	"stop-plugin",
	"unload-plugin",
	NULL
};

static cp_context_t *context = NULL;

static void error_handler(cp_context_t *context, const char *msg) {
	fprintf(stderr, "ERROR: %s\n", msg);
}

static char *state_to_string(cp_plugin_state_t state) {
	switch (state) {
		case CP_PLUGIN_UNINSTALLED:
			return "UNINSTALLED";
		case CP_PLUGIN_INSTALLED:
			return "INSTALLED";
		case CP_PLUGIN_RESOLVED:
			return "RESOLVED";
		case CP_PLUGIN_STARTING:
			return "STARTING";
		case CP_PLUGIN_STOPPING:
			return "STOPPING";
		case CP_PLUGIN_ACTIVE:
			return "ACTIVE";
		default:
			return "(unknown)";
	}
}

static void event_listener(cp_context_t *context, const cp_plugin_event_t *event) {
	printf("EVENT: plug-in %s: %s -> %s\n",
		event->plugin_id,
		state_to_string(event->old_state),
		state_to_string(event->new_state));
}

static void cmd_load_plugin(int argc, char *argv[]) {
	cp_plugin_t *plugin;
	int status;
	
	if (argc != 2) {
		fputs("ERROR: Usage: load-plugin <plugin directory>\n", stderr);
		return;
	} else if ((plugin = cp_load_plugin(context, argv[1], &status)) == NULL) {
		fprintf(stderr, "ERROR: cp_load_plugin failed with status code %d\n",
			status);
	} else {
		printf("Loaded plug-in %s version %s.\n",
			plugin->identifier, plugin->version);
		cp_release_plugin(plugin);
	}
}

static void cmd_list_plugins(int argc, char *argv[]) {
	cp_plugin_t **plugins;
	int status;
	
	if (argc != 1) {
		fputs("ERROR: Usage: list-plugins\n", stderr);
	} else if ((plugins = cp_get_plugins(context, &status, NULL)) == NULL) {
		fprintf(stderr, "ERROR: cp_get_plugins failed with status code %d\n",
			status);
	} else {
		int i;
		
		fputs("Installed plug-ins:\n", stdout);
		for (i = 0; plugins[i] != NULL; i++) {
			printf("  %s, version %s, state %s\n",
				plugins[i]->identifier,
				plugins[i]->version,
				state_to_string(cp_get_plugin_state(context, plugins[i]->identifier)));
		}
		cp_release_plugins(plugins);
	}
}

int main(int argc, char *argv[]) {
	int cmd;
	
	/* Create a context */
	cp_init();
	if ((context = cp_create_context(error_handler, NULL)) == NULL) {
		exit(1);
	}
	if (cp_add_event_listener(context, event_listener) != CP_OK) {
		exit(1);
	}
	
	/* Read commands and process them */
	do {
		char *l;
		
		l = readline("testpcontrol> ");
		if (l != NULL) {
			char *argv[16];
			int i, argc;
			
			/* Parse string to command and arguments */
			for (i = 0; isspace(l[i]); i++);
			for (argc = 0; l[i] != '\0' && argc < 16; argc++) {
				argv[argc] = l + i;
				while (l[i] != '\0' && !isspace(l[i])) {
					i++;
				}
				if (l[i] != '\0') {
					l[i++] = '\0';
					while (isspace(l[i])) {
						i++;
					}
				}
			}
			
			cmd = -1;
			if (argc > 0) {
				for (i = 0; commands[i] != NULL; i++) {
					if (!strcmp(l, commands[i])) {
						cmd = i;
						break;
					}
				}
				switch (cmd) {
					case 0: /* exit */
						break;
					case 3: /* load-plugin */
						cmd_load_plugin(argc, argv);
						break;
					case 5: /* list-plugins */
						cmd_list_plugins(argc, argv);
						break;
					default:
						fprintf(stderr, "ERROR: Unknown command %s\n", l);
						break;
				}
				add_history(l);
			}
			free(l);
		} else {
			cmd = 0;
		}
	} while (cmd != 0);
	
	/* Destroy context */
	cp_destroy_context(context);
	return 0;
}
