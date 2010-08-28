#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <set>
#include "internalxx.h"

namespace cpluff {


static fatal_error_handler *fatalErrorHandler = NULL;

static void invoke_fatalErrorHandler(const char *msg) {
	fatalErrorHandler->fatal_error(msg);
}

CP_CXX_API const char* CPFramework::getVersion() throw () {
	return cp_get_version();
}

CP_CXX_API const char* CPFramework::getHostType() throw () {
	return cp_get_host_type();
}

CP_CXX_API void CPFramework::setFatalErrorHandler(fatal_error_handler &feh) throw () {
	fatalErrorHandler = &feh;
	cp_set_fatal_error_handler(invoke_fatalErrorHandler);
}

CP_CXX_API void CPFramework::resetFatalErrorHandler() throw () {
	fatalErrorHandler = NULL;
	cp_set_fatal_error_handler(NULL);
}

CP_CXX_API CPFramework& CPFramework::init() {
	return *(new CPFrameworkImpl());
}


}
