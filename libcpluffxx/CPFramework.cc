#include <set>
#include "internalxx.h"

namespace org {
namespace cpluff {


static CPFatalErrorHandler *fatalErrorHandler = NULL;

static void invoke_fatalErrorHandler(const char *msg) {
	fatalErrorHandler->handleFatalError(msg);
}

CP_CXX_API const char* CPFramework::getVersion() throw () {
	return cp_get_version();
}

CP_CXX_API const char* CPFramework::getHostType() throw () {
	return cp_get_host_type();
}

CP_CXX_API void CPFramework::setFatalErrorHandler(CPFatalErrorHandler &feh) throw () {
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


}}
