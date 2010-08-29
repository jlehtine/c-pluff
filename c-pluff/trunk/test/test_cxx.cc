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
#include "test_cxx.h"

class full_logger : public cpluff::logger {
public:

	full_logger(int *error_counter): error_counter(error_counter) {}

	~full_logger() {}

	void log(severity sev, const char* msg, const char* apid) {
		const char *sevstr;
		switch (sev) {
			case DEBUG:
				sevstr = "DEBUG";
				break;
			case INFO:
				sevstr = "INFO";
				break;
			case WARNING:
				sevstr = "WARNING";
				break;
			case ERROR:
				sevstr = "ERROR";
				break;
			default:
				check((sevstr = "UNKNOWN", 0));
				break;
		}
		if (apid != NULL) {
			fprintf(stderr, "testsuite: %s: [%s] %s\n", sevstr, apid, msg);
		} else {
			fprintf(stderr, "testsuite: %s: [testsuite] %s\n", sevstr, msg);
		}
		if (sev >= ERROR && error_counter != NULL) {
			(*error_counter)++;
		}
	}

private:
	int *error_counter;
};

class counting_logger : public cpluff::logger {
public:
	counting_logger(int *error_counter): error_counter(error_counter) {}

	void log(severity sev, const char* msg, const char* apid) {
		(*error_counter)++;
	}

private:
	int *error_counter;
};

CP_HIDDEN cpluff::plugin_container *init_container_cxx(cpluff::logger::severity min_disp_sev, int *error_counter) {
	cpluff::framework::init();
	cpluff::plugin_container *pc = cpluff::framework::new_plugin_container();
	if (error_counter != NULL) {
		*error_counter = 0;
	}
	if (error_counter != NULL || min_disp_sev <= cpluff::logger::ERROR) {
		if (min_disp_sev <= cpluff::logger::ERROR) {
			pc->register_logger(new full_logger(error_counter), min_disp_sev);
		} else {
			pc->register_logger(new counting_logger(error_counter), cpluff::logger::ERROR);
		}
	}
	return pc;
}
