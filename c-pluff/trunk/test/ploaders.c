/*-------------------------------------------------------------------------
 * C-Pluff, a plug-in framework for C
 * Copyright 2007 Johannes Lehtinen
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *-----------------------------------------------------------------------*/

#include <stdio.h>
#include "test.h"

void oneploader(void) {
	cp_context_t *ctx;
	cp_plugin_loader_t *loader;
	cp_status_t status;
	int errors;

	ctx = init_context(CP_LOG_ERROR, &errors);
	loader = cp_create_local_ploader(&status);
	check(loader != NULL);
	check(status == CP_OK);
	check(cp_lpl_register_dir(loader, pcollectiondir("collection1")) == CP_OK);
	check(cp_scan_plugins(ctx, 0) == CP_OK);
	check(cp_get_plugin_state(ctx, "plugin1") == CP_PLUGIN_UNINSTALLED);
	check(cp_register_ploader(ctx, loader) == CP_OK);
	check(cp_get_plugin_state(ctx, "plugin1") == CP_PLUGIN_UNINSTALLED);
	check(cp_scan_plugins(ctx, 0) == CP_OK);
	check(cp_get_plugin_state(ctx, "plugin1") == CP_PLUGIN_INSTALLED);
	cp_destroy();
	check(errors == 0);
}

void twoploaders(void) {
	cp_context_t *ctx;
	cp_plugin_loader_t *loader1, *loader2;
	cp_status_t status;
	int errors;

	ctx = init_context(CP_LOG_ERROR, &errors);
	loader1 = cp_create_local_ploader(&status);
	check(loader1 != NULL);
	check(status == CP_OK);
	check(cp_lpl_register_dir(loader1, pcollectiondir("collection1")) == CP_OK);
	check(cp_register_ploader(ctx, loader1) == CP_OK);
	loader2 = cp_create_local_ploader(&status);
	check(loader2 != NULL);
	check(status == CP_OK);
	check(cp_register_ploader(ctx, loader2) == CP_OK);
	check(cp_lpl_register_dir(loader2, pcollectiondir("collection2")) == CP_OK);
	check(cp_scan_plugins(ctx, 0) == CP_OK);
	check(cp_get_plugin_state(ctx, "plugin1") == CP_PLUGIN_INSTALLED);
	check(cp_get_plugin_state(ctx, "plugin2a") == CP_PLUGIN_INSTALLED);
	check(cp_get_plugin_state(ctx, "plugin2b") == CP_PLUGIN_INSTALLED);
	cp_destroy();
	check(errors == 0);
}

void oneploadertwodirs(void) {
	cp_context_t *ctx;
	cp_plugin_loader_t *loader;
	cp_status_t status;
	int errors;

	ctx = init_context(CP_LOG_ERROR, &errors);
	loader = cp_create_local_ploader(&status);
	check(loader != NULL);
	check(status == CP_OK);
	check(cp_register_ploader(ctx, loader) == CP_OK);
	check(cp_lpl_register_dir(loader, pcollectiondir("collection1")) == CP_OK);
	check(cp_lpl_register_dir(loader, pcollectiondir("collection2")) == CP_OK);
	check(cp_scan_plugins(ctx, 0) == CP_OK);
	check(cp_get_plugin_state(ctx, "plugin1") == CP_PLUGIN_INSTALLED);
	check(cp_get_plugin_state(ctx, "plugin2a") == CP_PLUGIN_INSTALLED);
	check(cp_get_plugin_state(ctx, "plugin2b") == CP_PLUGIN_INSTALLED);
	cp_destroy();
	check(errors == 0);
}

void ploaderunregdir(void) {
	cp_context_t *ctx;
	cp_plugin_loader_t *loader;
	cp_status_t status;
	int errors;

	ctx = init_context(CP_LOG_ERROR, &errors);
	loader = cp_create_local_ploader(&status);
	check(loader != NULL);
	check(status == CP_OK);
	check(cp_register_ploader(ctx, loader) == CP_OK);
	check(cp_lpl_register_dir(loader, pcollectiondir("collection1")) == CP_OK);
	check(cp_lpl_register_dir(loader, pcollectiondir("collection2")) == CP_OK);
	cp_lpl_unregister_dir(loader, pcollectiondir("collection1"));
	check(cp_scan_plugins(ctx, 0) == CP_OK);
	check(cp_get_plugin_state(ctx, "plugin1") == CP_PLUGIN_UNINSTALLED);
	check(cp_get_plugin_state(ctx, "plugin2a") == CP_PLUGIN_INSTALLED);
	check(cp_get_plugin_state(ctx, "plugin2b") == CP_PLUGIN_INSTALLED);
	cp_destroy();
	check(errors == 0);
}
