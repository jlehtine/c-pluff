#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <set>
#include "internalxx.h"

namespace cpluff {


CP_HIDDEN framework_impl::framework_impl() {
	
	// Initialize the framework
	check_cp_status(cp_init());
}

CP_HIDDEN framework_impl::~framework_impl() throw () {
	
	// Destroy remaining plug-in containers
	while (!containers.empty()) {
		(*(containers.begin()))->destroy();
	}
	
	// Destroy the framework
	cp_destroy();
}

CP_HIDDEN void framework_impl::destroy() throw () {
	delete this;
}

CP_HIDDEN plugin_container& framework_impl::create_plugin_container() throw (api_error) {
	plugin_container_impl& container = *(new plugin_container_impl(*this));
	// TODO: synchronization
	containers.insert(&container);
	return container;
}

CP_HIDDEN void framework_impl::unregister_plugin_container(plugin_container_impl& container) throw () {
	// TODO: synchronization
	containers.erase(&container);
}


}
