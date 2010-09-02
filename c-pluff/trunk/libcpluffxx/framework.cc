#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

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

shared_ptr<framework> framework::init() throw (api_error) {
	shared_ptr<framework_impl> sp(new framework_impl);
	sp.get()->this_shared(sp);
	return sp;
}

CP_HIDDEN shared_ptr<plugin_container> framework_impl::new_plugin_container() throw (api_error) {
	return shared_ptr<plugin_container>(new plugin_container_impl(shared_ptr<framework>(this_weak)));
}

}
