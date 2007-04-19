#include <cstdarg>
#include <cassert>
#include <cpluff.h>
#include "internalxx.h"

namespace org {
namespace cpluff {


CP_HIDDEN CPPluginContextImpl::CPPluginContextImpl()
: minLoggerSeverity(static_cast<CPLogger::Severity>(CPLogger::ERROR + 1)) {
	cp_status_t status;
	context = cp_create_context(&status);
	util::checkStatus(status);
}

CP_HIDDEN CPPluginContextImpl::~CPPluginContextImpl() throw () {
	cp_destroy_context(context);
}

CP_HIDDEN void CPPluginContextImpl::registerLogger(CPLogger &logger, CPLogger::Severity minSeverity) throw (CPAPIError) {
	// TODO synchronization
	loggers[&logger] = minSeverity;
	updateMinLoggerSeverity();
}

CP_HIDDEN void CPPluginContextImpl::unregisterLogger(CPLogger &logger) throw () {
	// TODO synchronization
	loggers.erase(&logger);
	updateMinLoggerSeverity();
}

CP_HIDDEN void CPPluginContextImpl::log(CPLogger::Severity severity, const char* msg) throw () {
	cp_log(context, (cp_log_severity_t) severity, msg);
}

CP_HIDDEN bool CPPluginContextImpl::isLogged(CPLogger::Severity severity) throw () {
	return cp_is_logged(context, (cp_log_severity_t) severity);
}

CP_HIDDEN void CPPluginContextImpl::logf(CPLogger::Severity severity, const char* msg, ...) throw () {
	assert(msg != NULL);
	assert(severity >= CP_LOG_DEBUG && severity <= CP_LOG_ERROR);

	if (isLogged(severity)) {
		char buffer[256];
		va_list va;
	
		va_start(va, msg);
		vsnprintf(buffer, sizeof(buffer), _(msg), va);
		va_end(va);
		strcpy(buffer + sizeof(buffer)/sizeof(char) - 4, "...");
		cp_log(context, (cp_log_severity_t) severity, buffer);
	}	
}

CP_HIDDEN void CPPluginContextImpl::deliverLogMessage(cp_log_severity_t sev, const char* msg, const char* apid, void* user_data) throw () {
	CPPluginContextImpl* context = static_cast<CPPluginContextImpl*>(user_data);
	std::map<CPLogger*, CPLogger::Severity>::iterator iter;
	// TODO synchronization
	for (iter = context->loggers.begin(); iter != context->loggers.end(); iter++) {
		std::pair<CPLogger* const, CPLogger::Severity>& p = *iter;
		CPLogger::Severity severity = static_cast<CPLogger::Severity>(sev);
		if (severity >= p.second) { 
			(p.first)->log(severity, msg, apid);
		}
	}
}

CP_HIDDEN void CPPluginContextImpl::updateMinLoggerSeverity() throw () {
	minLoggerSeverity = static_cast<CPLogger::Severity>(CPLogger::ERROR + 1);
	std::map<CPLogger*, CPLogger::Severity>::iterator iter;
	// TODO synchronization
	for (iter = loggers.begin(); iter != loggers.end(); iter++) {
		std::pair<CPLogger* const, CPLogger::Severity>& p = *iter;
		if (p.second < minLoggerSeverity) {
			minLoggerSeverity = p.second;
		}
	}
} 

}}
