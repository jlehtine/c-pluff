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
 * Declares exceptions.
 */

#ifndef CPLUFFXX_EXCEPT_H_
#define CPLUFFXX_EXCEPT_H_

#include <cpluffxx/defines.h>

namespace cpluff {

/**
 * Thrown to indicate an error in a C-Pluff API call. API functions that may
 * fail throw this exception on error conditions. This class is not intended
 * to be instantiated or subclassed by the client program.
 */
class cp_api_error {
public:

	/** Error codes included in cp_api_error. */
	enum code {
		
		/** Not enough memory or other operating system resources available */
		RESOURCE = 1,

		/** The specified object is unknown to the framework */
		UNKNOWN,

		/** An I/O error occurred */
		IO,

		/** Malformed plug-in descriptor was encountered when loading a plug-in */
		MALFORMED,

		/** Plug-in or symbol conflicts with another plug-in or symbol */
		CONFLICT,

		/** Plug-in dependencies could not be satisfied */
		DEPENDENCY,

		/** Plug-in runtime signaled an error */
		RUNTIME

	};

	/**
	 * @internal
	 * Constructs a new instance from the specified error code and error
	 * message. The error message is not copied so the pointer should point to
	 * static storage or be valid for the life time of the created object.
	 * 
	 * @param reason the error code
	 * @param message the error message
	 */
	CP_HIDDEN inline cp_api_error(code reason, const char* message):
	error_code(reason), error_message(message) {};

	/**
	 * Returns an error code describing the type of error.
	 * 
	 * @return an error code describing the type of error
	 */ 
	CP_HIDDEN inline code reason() const {
		return error_code;
	}

	/**
	 * Returns an error message intended for display purposes. The error
	 * message may be localized. The returned error message is valid until this
	 * exception object is destructed.
	 * 
	 * @return a possibly localized error message intended for display purposes
	 */ 
	CP_HIDDEN inline const char* message() const {
		return error_message;
	}

protected:

	/** @internal The associated error code */
	code error_code;
	
	/** @internal The associated error message */
	const char* error_message;
};

}

#endif /*CPLUFFXX_EXCEPT_H_*/
