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

#include <cstdio>
#include <cstdlib>
#include <cpluffxx.h>
#include "test.h"

static int testvar;

static void cause_fatal_error(void) {
	cp_context_t *ctx;
	
	// TODO: Replace with C++ API implementation
	cp_init();
	ctx = init_context((cp_log_severity_t) (CP_LOG_ERROR + 1), NULL);
	cp_release_info(ctx, &testvar);
	cp_destroy();
}

extern "C" void fatalerrordefault_cxx(void) {
	cause_fatal_error();
}

class test_error_handler : public cpluff::fatal_error_handler {
public:
	void fatal_error(const char* msg) {
		free_test_resources();
		exit(0);
	}
};

extern "C" void fatalerrorhandled_cxx(void) {
	test_error_handler eh;
	cpluff::framework::set_fatal_error_handler(eh);
	cause_fatal_error();
	free_test_resources();
	exit(1);
}

extern "C" void fatalerrorreset_cxx(void) {
	test_error_handler eh;
	cpluff::framework::set_fatal_error_handler(eh);
	cpluff::framework::reset_fatal_error_handler();
	cause_fatal_error();
}
