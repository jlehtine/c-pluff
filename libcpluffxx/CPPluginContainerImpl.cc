#include <cstdarg>
#include <cassert>
#include <cpluff.h>
#include "internalxx.h"

namespace org {
namespace cpluff {


CP_HIDDEN CPPluginContainerImpl::CPPluginContainerImpl(CPFrameworkImpl& framework)
: framework(framework) {
	cp_status_t status;
	context = cp_create_context(&status);
	util::checkStatus(status);
	this->context = context;
}

CP_HIDDEN CPPluginContainerImpl::~CPPluginContainerImpl() throw () {
	cp_destroy_context(context);
}

CP_HIDDEN void CPPluginContainerImpl::destroy() throw () {
	framework.unregisterPluginContainer(*this);
	delete this;
}

CP_HIDDEN CPPluginDescriptor& CPPluginContainerImpl::loadPluginDescriptor(const char* path) {
	cp_status_t status;
	cp_plugin_info_t *pinfo = cp_load_plugin_descriptor(context, path, &status);
	util::checkStatus(status);
}

}}
