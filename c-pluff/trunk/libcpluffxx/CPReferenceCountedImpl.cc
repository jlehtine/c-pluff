#include <cassert>
#include "internalxx.h"

namespace org {
namespace cpluff {


CP_HIDDEN CPReferenceCountedImpl::CPReferenceCountedImpl(CPPluginContextImpl& context) throw ()
:referenceCount(1), context(context) {
	context.logf(CPLogger::DEBUG, N_("Constructed a new reference counted object at address %p."), this);
};

CP_HIDDEN CPReferenceCountedImpl::~CPReferenceCountedImpl() throw () {
	if (referenceCount != 0) {
		cpi_fatalf(_("Attempt to destruct a non-released reference counted object at address %p."), this);
	}
	context.logf(CPLogger::DEBUG, N_("Destructed the reference counted object at address %p."), this);
}

CP_HIDDEN void CPReferenceCountedImpl::use() throw () {
	assert(referenceCount > 0);
	referenceCount++;
	context.logf(CPLogger::DEBUG, N_("Reference count of the object at address %p increased to %d."), this, referenceCount);
}

CP_HIDDEN void CPReferenceCountedImpl::release() throw () {
	if (referenceCount <= 0) {
		cpi_fatalf(_("Attempt to release an already destructed reference counted object at address %p."), this);
	}
	referenceCount--;
	context.logf(CPLogger::DEBUG, N_("Reference count of the object at address %p decreased to %d."), this, referenceCount);
	if (referenceCount == 0) {
		delete this;
	}
}


}}
