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

const char* framework::version() throw () {
	return cp_get_version();
}

const char* framework::host_type() throw () {
	return cp_get_host_type();
}

void framework::fatal_error_handler(::cpluff::fatal_error_handler &feh) throw () {
	current_fatal_error_handler = &feh;
	cp_set_fatal_error_handler(invoke_fatal_error_handler);
}

void framework::reset_fatal_error_handler() throw () {
	current_fatal_error_handler = NULL;
	cp_set_fatal_error_handler(NULL);
}

void framework::init() throw (api_error) {
	check_cp_status(cp_init());
}

void framework::destroy() {
	cp_destroy();
}


}
