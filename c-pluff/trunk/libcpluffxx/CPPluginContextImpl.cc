#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cstdarg>
#include <cassert>
#include <cpluff.h>
#include "internalxx.h"

namespace cpluff {


CP_HIDDEN CPPluginContextImpl::CPPluginContextImpl(cp_context_t *context)
: context(context),
  minLoggerseverity(static_cast<logger::severity>(logger::ERROR + 1)) {}

CP_HIDDEN CPPluginContextImpl::CPPluginContextImpl() {
	CPPluginContextImpl(NULL);
}

CP_HIDDEN CPPluginContextImpl::~CPPluginContextImpl() throw () {
	cp_destroy_context(context);
}

CP_HIDDEN void CPPluginContextImpl::registerLogger(logger &logger, logger::severity minseverity) throw (cp_api_error) {
	// TODO synchronization
	loggers[&logger] = minseverity;
	updateMinLoggerseverity();
}

CP_HIDDEN void CPPluginContextImpl::unregisterLogger(logger &logger) throw () {
	// TODO synchronization
	loggers.erase(&logger);
	updateMinLoggerseverity();
}

CP_HIDDEN void CPPluginContextImpl::log(logger::severity severity, const char* msg) throw () {
	cp_log(context, (cp_log_severity_t) severity, msg);
}

CP_HIDDEN bool CPPluginContextImpl::isLogged(logger::severity severity) throw () {
	return cp_is_logged(context, (cp_log_severity_t) severity);
}

CP_HIDDEN void CPPluginContextImpl::logf(logger::severity severity, const char* msg, ...) throw () {
	assert(msg != NULL);
	assert(severity >= CP_LOG_DEBUG && severity <= CP_LOG_ERROR);

	if (isLogged(severity)) {
		char buffer[256];
		va_list va;
	
		va_start(va, msg);
		// TODO fix vsnprintf(buffer, sizeof(buffer), _(msg), va);
		va_end(va);
		strcpy(buffer + sizeof(buffer)/sizeof(char) - 4, "...");
		cp_log(context, (cp_log_severity_t) severity, buffer);
	}	
}

CP_HIDDEN void CPPluginContextImpl::deliverLogMessage(cp_log_severity_t sev, const char* msg, const char* apid, void* user_data) throw () {
	CPPluginContextImpl* context = static_cast<CPPluginContextImpl*>(user_data);
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

CP_HIDDEN void CPPluginContextImpl::updateMinLoggerseverity() throw () {
	minLoggerseverity = static_cast<logger::severity>(logger::ERROR + 1);
	std::map<logger*, logger::severity>::iterator iter;
	// TODO synchronization
	for (iter = loggers.begin(); iter != loggers.end(); iter++) {
		std::pair<logger* const, logger::severity>& p = *iter;
		if (p.second < minLoggerseverity) {
			minLoggerseverity = p.second;
		}
	}
} 

}
