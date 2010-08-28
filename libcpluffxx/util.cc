/*-------------------------------------------------------------------------
 * C-Pluff, a plug-in framework for C
 * Copyright 2007 Johannes Lehtinen
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *-----------------------------------------------------------------------*/

/** @file 
 * Implements internal utility functions for the C-Pluff C++ implementation.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "internalxx.h"

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

CP_HIDDEN void check_cp_status(cp_status_t status) {
	if (status != CP_OK) {
		throw cp_api_error(
			(cp_api_error::code) status,
			statusToCString(status)
		);
	}
}

}
