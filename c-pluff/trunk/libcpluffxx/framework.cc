#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <set>
#include "internalxx.h"

namespace cpluff {


static fatal_error_handler *current_fatal_error_handler = NULL;

static void invoke_fatal_error_handler(const char *msg) {
	current_fatal_error_handler->fatal_error(msg);
}

CP_CXX_API const char* framework::version() throw () {
	return cp_get_version();
}

CP_CXX_API const char* framework::host_type() throw () {
	return cp_get_host_type();
}

CP_CXX_API void framework::fatal_error_handler(fatal_error_handler &feh) throw () {
	current_fatal_error_handler = &feh;
	cp_set_fatal_error_handler(invoke_fatal_error_handler);
}

CP_CXX_API void framework::reset_fatal_error_handler() throw () {
	current_fatal_error_handler = NULL;
	cp_set_fatal_error_handler(NULL);
}

CP_CXX_API framework& framework::init() {
	return *(new framework_impl());
}


}
