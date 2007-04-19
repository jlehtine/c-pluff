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
 * C-Pluff C++ implementation internal header file.
 */

#ifndef INTERNALXX_H_
#define INTERNALXX_H_

#include <set>
#include <map>
#include <cpluff.h>
#include "cpluffxx.h"
#include "../libcpluff/defines.h"
#include "../libcpluff/shared.h"

namespace org {
namespace cpluff {

/* -----------------------------------------------------------------------
 * Internal utility functions
 * ---------------------------------------------------------------------*/
 
namespace util {

/**
 * @internal
 * Throws a generic exception matching the specified status code. Does nothing
 * if the specified status code is CP_OK.
 * 
 * @param status the status code from C API
 * @throw CPAPIError if the status code indicates a failure
 */
CP_HIDDEN void checkStatus(cp_status_t status) throw (CPAPIError);

}


/* -----------------------------------------------------------------------
 * Implementation classes
 * ---------------------------------------------------------------------*/

class CPPluginContextImpl;
class CPPluginContainerImpl;

class CPAPIErrorImpl : public CPAPIError {
public:

	/**
	 * Constructs an exception with the specified error code and message
	 * specified as C string. The error message is stored by pointer.
	 * 
	 * @param errorCode the associated error code
	 * @param errorMsg the associated error message
	 */
	CP_HIDDEN CPAPIErrorImpl(ErrorCode errorCode, const char* errorMsg) throw ();

	CP_HIDDEN ErrorCode getErrorCode() const throw ();

	CP_HIDDEN const char* getErrorMessage() const throw ();

private:

	/**
	 * The associated error code
	 */
	const ErrorCode errorCode;
	
	/**
	 * The associated localized error message
	 */
	const char* errorMessage;
	
};

class CPReferenceCountedImpl : public virtual CPReferenceCounted {
public:

	CP_HIDDEN void use() throw ();
	
	CP_HIDDEN void release() throw ();	

protected:

	/**
	 * Constructs a new reference counted object and initializes its reference
	 * count to one.
	 * 
	 * @param context the associated plug-in context
	 */
	CP_HIDDEN CPReferenceCountedImpl(CPPluginContextImpl& context) throw ();

	/**
	 * Destructs a reference counted object.
	 */
	CP_HIDDEN ~CPReferenceCountedImpl() throw ();

private:

	/**
	 * @internal
	 * The current reference count.
	 */
	int referenceCount;

	/**
	 * @internal
	 * The associated plug-in context.
	 */
	CPPluginContextImpl& context;
	
};

class CPFrameworkImpl : public CPFramework {
public:

	/**
	 * Actually initializes the framework and constructs a framework object.
	 */
	CP_HIDDEN CPFrameworkImpl();

	/**
	 * Destructs a framework object and deinitializes the framework if this
	 * was the last object.
	 */
	CP_HIDDEN ~CPFrameworkImpl() throw ();

	CP_HIDDEN void destroy() throw ();

	CP_HIDDEN CPPluginContainer& createPluginContainer() throw (CPAPIError);

	/**
	 * Unregisters a plug-in container object with this framework.
	 * After the plug-in container has been unregistered, it is not implicitly
	 * destroyed when the framework is destroyed.
	 * 
	 * @param container the container to be unregistered
	 */
	CP_HIDDEN void unregisterPluginContainer(CPPluginContainerImpl& container) throw ();

private:

	/** 
	 * The associated valid plug-in containers
	 */
	std::set<CPPluginContainerImpl*> containers;

};

class CPPluginContextImpl : public virtual CPPluginContext {
public:

	/**
	 * Constructs a new plug-in context.
	 * 
	 * @throw CPAPIError if an error occurs
	 */
	CP_HIDDEN CPPluginContextImpl();

	CP_HIDDEN void registerLogger(CPLogger& logger, CPLogger::Severity minSeverity) throw (CPAPIError);

	CP_HIDDEN void unregisterLogger(CPLogger& logger) throw ();

	CP_HIDDEN void log(CPLogger::Severity severity, const char* msg) throw ();

	CP_HIDDEN bool isLogged(CPLogger::Severity severity) throw ();

	/**
	 * Emits a new formatted log message if the associated severity is being
	 * logged. Uses gettext to translate the message.
	 * 
	 * @param severity the severity of the event
	 * @param msg the log message (possibly localized)
	 */
	CP_HIDDEN void logf(CPLogger::Severity severity, const char* msg, ...) throw ();

protected:

	/**
	 * Handle to the underlying C API plug-in context.
	 */
	cp_context_t* context;

protected:

	/**
	 * Destructs a plug-in context. All resources associated with this
	 * plug-in context are released and all pointers and references
	 * obtained via it become invalid.
	 */
	CP_HIDDEN ~CPPluginContextImpl() throw ();

private:

	/**
	 * The registered loggers and their minimum logging severity.
	 */
	std::map<CPLogger*, CPLogger::Severity> loggers;

	/**
	 * The minimum logging severity for registered loggers.
	 */
	CPLogger::Severity minLoggerSeverity;

	/**
	 * Delivers a logged message to all registered C++ loggers of a specific
	 * plug-in context object.
	 * 
	 * @param severity the severity of the message
	 * @param msg the message to be logged, possibly localized
	 * @param apid the identifier of the activating plug-in or NULL for the main program
	 * @param user_data pointer to the associated plug-in context object
	 */
	CP_HIDDEN static void deliverLogMessage(cp_log_severity_t severity, const char* msg, const char* apid, void* user_data) throw ();

	/**
	 * Updates the aggregate minimum severity for installed loggers.
	 */
	CP_HIDDEN void updateMinLoggerSeverity() throw (); 
};

class CPPluginContainerImpl : public CPPluginContainer, public CPPluginContextImpl {
public:

	CP_HIDDEN CPPluginContainerImpl(CPFrameworkImpl& framework);
	
	CP_HIDDEN ~CPPluginContainerImpl() throw ();

	CP_HIDDEN void destroy() throw ();

	CP_HIDDEN void registerPluginCollection(const char* dir) throw (CPAPIError);

	CP_HIDDEN void unregisterPluginCollection(const char* dir) throw ();

	CP_HIDDEN void unregisterPluginCollections() throw ();

private:

	/**
	 * The associated framework object
	 */
	CPFrameworkImpl& framework;
	
};

}}

#endif /*INTERNALXX_H_*/
