#include <stdio.h>
#include "test.h"
#include <cpluff.h>

void initdestroy(void) {
	int i;
	
	for (i = 0; i < 10; i++) {
		check(cp_init() == CP_OK);
		cp_destroy();
	}
}

void initcreatedestroy(void) {
	int i;
	
	for (i = 0; i < 10; i++) {
		int errors;
		
		init_context(CP_LOG_ERROR, &errors);
		cp_destroy();
		check(errors == 0);
	}
}

void initloaddestroy(void) {
	int i;
	
	for (i = 0; i < 10; i++) {
		cp_context_t *ctx;
		cp_plugin_info_t *pi;
		cp_status_t status;
		const char *pdir = plugindir("minimal");
		int errors;

		ctx = init_context(CP_LOG_ERROR, &errors);		
		check((pi = cp_load_plugin_descriptor(ctx, pdir, &status)) != NULL && status == CP_OK);
		cp_release_info(ctx, pi);
		cp_destroy();
		check(errors == 0);
	}
}

void initinstalldestroy(void) {
	int i;
	
	for (i = 0; i < 10; i++) {
		cp_context_t *ctx;
		cp_plugin_info_t *pi;
		cp_status_t status;
		const char *pdir = plugindir("minimal");
		int errors;

		ctx = init_context(CP_LOG_ERROR, &errors);		
		check((pi = cp_load_plugin_descriptor(ctx, pdir, &status)) != NULL && status == CP_OK);
		check(cp_install_plugin(ctx, pi) == CP_OK);
		cp_release_info(ctx, pi);
		cp_destroy();
		check(errors == 0);
	}
}

void initstartdestroy(void) {
	int i;
	
	for (i = 0; i < 10; i++) {
		cp_context_t *ctx;
		cp_plugin_info_t *pi;
		cp_status_t status;
		const char *pdir = plugindir("minimal");
		int errors;
		
		ctx = init_context(CP_LOG_ERROR, &errors);
		check((pi = cp_load_plugin_descriptor(ctx, pdir, &status)) != NULL && status == CP_OK);
		check(cp_install_plugin(ctx, pi) == CP_OK);
		cp_release_info(ctx, pi);
		check(cp_start_plugin(ctx, "minimal") == CP_OK);
		cp_destroy();
		check(errors == 0);
	}
}

void initstartdestroyboth(void) {
	int i;
	
	for (i = 0; i < 10; i++) {
		cp_context_t *ctx;
		cp_plugin_info_t *pi;
		cp_status_t status;
		const char *pdir = plugindir("minimal");
		int errors;
		
		ctx = init_context(CP_LOG_ERROR, &errors);
		check((pi = cp_load_plugin_descriptor(ctx, pdir, &status)) != NULL && status == CP_OK);
		check(cp_install_plugin(ctx, pi) == CP_OK);
		cp_release_info(ctx, pi);
		check(cp_start_plugin(ctx, "minimal") == CP_OK);
		cp_destroy_context(ctx);
		cp_destroy();
		check(errors == 0);
	}
}
