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

/** @file
 * Plug-in context implementation.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cstdarg>
#include <cstdio>
#include <cassert>
#include <cpluff.h>
#include "internalxx.h"

namespace cpluff {


CP_HIDDEN plugin_context_impl::plugin_context_impl(cp_context_t *context)
: context(context),
  min_logger_severity(static_cast<logger::severity>(logger::ERROR + 1)) {}

CP_HIDDEN plugin_context_impl::plugin_context_impl() {
	plugin_context_impl(NULL);
}

CP_HIDDEN plugin_context_impl::~plugin_context_impl() throw () {
	cp_destroy_context(context);
}

CP_HIDDEN void plugin_context_impl::register_logger(logger* logger, logger::severity minseverity) throw (api_error) {
	// TODO synchronization
	loggers[logger] = minseverity;
	update_min_logger_severity();
}

CP_HIDDEN void plugin_context_impl::unregister_logger(logger* logger) throw () {
	// TODO synchronization
	loggers.erase(logger);
	update_min_logger_severity();
}

CP_HIDDEN void plugin_context_impl::log(logger::severity severity, const char* msg) throw () {
	cp_log(context, (cp_log_severity_t) severity, msg);
}

CP_HIDDEN bool plugin_context_impl::is_logged(logger::severity severity) throw () {
	return cp_is_logged(context, (cp_log_severity_t) severity);
}

CP_HIDDEN void plugin_context_impl::logf(logger::severity severity, const char* msg, ...) throw () {
	assert(msg != NULL);
	assert(severity >= logger::DEBUG && severity <= logger::ERROR);

	if (is_logged(severity)) {
		char buffer[256];
		va_list va;
	
		va_start(va, msg);
		vsnprintf(buffer, sizeof(buffer), _(msg), va);
		va_end(va);
		strcpy(buffer + sizeof(buffer)/sizeof(char) - 4, "...");
		cp_log(context, (cp_log_severity_t) severity, buffer);
	}	
}

CP_HIDDEN void plugin_context_impl::deliver_log_message(cp_log_severity_t sev, const char* msg, const char* apid, void* user_data) throw () {
	plugin_context_impl* context = static_cast<plugin_context_impl*>(user_data);
	std::map<logger*, logger::severity>::iterator iter;
	// TODO synchronization
	for (iter = context->loggers.begin(); iter != context->loggers.end(); iter++) {
		std::pair<logger* const, logger::severity>& p = *iter;
		logger::severity severity = static_cast<logger::severity>(sev);
		if (severity >= p.second) { 
			(p.first)->log(severity, msg, apid);
		}
	}
}

CP_HIDDEN void plugin_context_impl::update_min_logger_severity() throw () {
	min_logger_severity = static_cast<logger::severity>(logger::ERROR + 1);
	std::map<logger*, logger::severity>::iterator iter;
	// TODO synchronization
	for (iter = loggers.begin(); iter != loggers.end(); iter++) {
		std::pair<logger* const, logger::severity>& p = *iter;
		if (p.second < min_logger_severity) {
			min_logger_severity = p.second;
		}
	}
} 

}
