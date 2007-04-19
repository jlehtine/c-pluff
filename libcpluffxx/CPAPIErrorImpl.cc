#include <string>
#include "internalxx.h"

namespace org {
namespace cpluff {


/**
 * Converts a status code to an error string.
 * 
 * @param status a status code from C API
 * @return corresponding error message as C string
 */
static const char* statusToCString(cp_status_t status) throw() {
	switch (status) {
		case CP_ERR_RESOURCE:
			return _("Insufficient system resources for the operation.");
		case CP_ERR_UNKNOWN:
			return _("An unknown object was specified.");
		case CP_ERR_IO:
			return _("An input or output error occurred.");
		case CP_ERR_MALFORMED:
			return _("Encountered a malformed plug-in descriptor.");
		case CP_ERR_CONFLICT:
			return _("A plug-in or symbol conflicts with an existing one.");
		case CP_ERR_DEPENDENCY:
			return _("Plug-in dependencies can not be satisfied.");
		case CP_ERR_RUNTIME:
			return _("A plug-in runtime library encountered an error.");
		default:
			return _("An unknown error occurred.");
	}	
}

CP_HIDDEN void util::checkStatus(cp_status_t status) throw (CPAPIError) {
	if (status != CP_OK) {
		throw new CPAPIErrorImpl(
			(CPAPIError::ErrorCode) status,
			statusToCString(status));
	}
}

CP_HIDDEN CPAPIErrorImpl::CPAPIErrorImpl(ErrorCode errorCode, const char* errorMsg) throw (): errorCode(errorCode), errorMessage(errorMsg) {}

CP_HIDDEN CPAPIError::ErrorCode CPAPIErrorImpl::getErrorCode() const throw () {
	return errorCode;
};

CP_HIDDEN const char* CPAPIErrorImpl::getErrorMessage() const throw () {
	return errorMessage;
}

}}
