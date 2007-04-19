#include <set>
#include "internalxx.h"

namespace org {
namespace cpluff {


CP_HIDDEN CPFrameworkImpl::CPFrameworkImpl() {
	
	// Initialize the framework
	util::checkStatus(cp_init());
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

CP_HIDDEN CPPluginContainer& CPFrameworkImpl::createPluginContainer() throw (CPAPIError) {
	CPPluginContainerImpl& container = *(new CPPluginContainerImpl(*this));
	// TODO: synchronization
	containers.insert(&container);
	return container;
}

CP_HIDDEN void CPFrameworkImpl::unregisterPluginContainer(CPPluginContainerImpl& container) throw () {
	// TODO: synchronization
	containers.erase(&container);
}


}}
