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
}

CP_HIDDEN CPPluginContainerImpl::~CPPluginContainerImpl() throw () {
	cp_destroy_context(context);
}

CP_HIDDEN void CPPluginContainerImpl::destroy() throw () {
	framework.unregisterPluginContainer(*this);
	delete this;
}

}}
