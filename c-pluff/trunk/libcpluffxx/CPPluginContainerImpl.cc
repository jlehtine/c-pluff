#include <cstdarg>
#include <cassert>
#include <cpluff.h>
#include "internalxx.h"

namespace cpluff {


CP_HIDDEN CPPluginContainerImpl::CPPluginContainerImpl(CPFrameworkImpl& framework)
: framework(framework) {
	cp_status_t status;
	context = cp_create_context(&status);
	check_cp_status(status);
	this->context = context;
}

CP_HIDDEN CPPluginContainerImpl::~CPPluginContainerImpl() throw () {
	cp_destroy_context(context);
}

CP_HIDDEN void CPPluginContainerImpl::destroy() throw () {
	framework.unregisterPluginContainer(*this);
	delete this;
}

CP_HIDDEN plugin_info& CPPluginContainerImpl::loadPluginDescriptor(const char* path) {
	cp_status_t status;
	cp_plugin_info_t *pinfo = cp_load_plugin_descriptor(context, path, &status);
	check_cp_status(status);
}

}
