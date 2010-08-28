#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <set>
#include "internalxx.h"

namespace cpluff {


CP_HIDDEN CPFrameworkImpl::CPFrameworkImpl() {
	
	// Initialize the framework
	check_cp_status(cp_init());
}

CP_HIDDEN CPFrameworkImpl::~CPFrameworkImpl() throw () {
	
	// Destroy remaining plug-in containers
	while (!containers.empty()) {
		(*(containers.begin()))->destroy();
	}
	
	// Destroy the framework
	cp_destroy();
}

CP_HIDDEN void CPFrameworkImpl::destroy() throw () {
	delete this;
}

CP_HIDDEN CPPluginContainer& CPFrameworkImpl::createPluginContainer() throw (cp_api_error) {
	CPPluginContainerImpl& container = *(new CPPluginContainerImpl(*this));
	// TODO: synchronization
	containers.insert(&container);
	return container;
}

CP_HIDDEN void CPFrameworkImpl::unregisterPluginContainer(CPPluginContainerImpl& container) throw () {
	// TODO: synchronization
	containers.erase(&container);
}


}
