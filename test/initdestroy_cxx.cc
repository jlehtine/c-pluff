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
#include <cpluffxx.h>
#include "test_cxx.h"

extern "C" void initdestroy_cxx(void) {
	for (int i = 0; i < 10; i++) {
		cpluff::framework::init();
	}
}

extern "C" void initcreatedestroy_cxx(void) {
	for (int i = 0; i < 3; i++) {
		int errors;
		do {
			init_container_cxx(cpluff::logger::ERROR, &errors);
		} while (0);
		check(errors == 0);
	}
}

extern "C" void initloaddestroy_cxx(void) {
	int i;
	
	for (i = 0; i < 3; i++) {
		const char *pdir = plugindir("minimal");
		int errors;
		do {
			shared_ptr<cpluff::plugin_container> pc = init_container_cxx(cpluff::logger::ERROR, &errors);
			pc.get()->load_plugin_descriptor(pdir);
		} while (0);
		check(errors == 0);
	}
}

extern "C" void initinstalldestroy_cxx(void) {
	int i;
	
	for (i = 0; i < 3; i++) {
		const char *pdir = plugindir("minimal");
		int errors;
		do {
			shared_ptr<cpluff::plugin_container> pc = init_container_cxx(cpluff::logger::ERROR, &errors);
			shared_ptr<cpluff::plugin_info> pi = pc.get()->load_plugin_descriptor(pdir);
			// TODO check(cp_install_plugin(ctx, pi) == CP_OK);
		} while (0);
		check(errors == 0);
	}
}

#if 0
extern "C" void initstartdestroy_cxx(void) {
	int i;
	
	for (i = 0; i < 3; i++) {
		cp_context_t *ctx;
		cp_plugin_info_t *pi;
		cp_status_t status;
		const char *pdir = plugindir("minimal");
		int errors;
		do {
		ctx = init_context(CP_LOG_ERROR, &errors);
		check((pi = cp_load_plugin_descriptor(ctx, pdir, &status)) != NULL && status == CP_OK);
		check(cp_install_plugin(ctx, pi) == CP_OK);
		cp_release_info(ctx, pi);
		check(cp_start_plugin(ctx, "minimal") == CP_OK);
		cp_destroy();
		} while (0);
		check(errors == 0);
	}
}

extern "C" void initstartdestroyboth_cxx(void) {
	int i;
	
	for (i = 0; i < 3; i++) {
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
#endif
