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
 * C-Pluff C++ API header file.
 * The elements declared here and in included header files constitute the
 * C-Pluff C++ API. To use the API include this file and link the main program
 * and plug-in runtime libraries with the C-Pluff C++ library. In addition to
 * C++ declarations, this file also includes cpluffdef.h header file for
 * defines common to C and C++ API.
 * 
 * At the moment this API is work in progress and changes are likely.
 */

#ifndef CPLUFFXX_H_
#define CPLUFFXX_H_

#include <cpluffxx/defines.h>
#include <cpluffxx/except.h>
#include <cpluffxx/enums.h>
#include <cpluffxx/callbacks.h>
#include <cpluffxx/info.h>


/* ------------------------------------------------------------------------
 * Classes
 * ----------------------------------------------------------------------*/

namespace cpluff {

class plugin_context;
class plugin_container;
class plugin_import;
class ext_point_info;
class extension_info;
class cfg_element;

/**
 * The core class used for global initialization and to access global
 * framework functionality. It also provides static information about
 * the framework implementation. This class is not intended to be
 * subclassed by the client program.
 */
class framework {
public:

	/**
	 * Returns the release version string of the linked in C-Pluff
	 * implementation.
	 * 
	 * @return the release version of the C-Pluff implementation 
	 */
	static const char* version() throw ();

	/**
	 * Returns the canonical host type associated with the linked in
	 * C-Pluff implementation. A multi-platform installation manager could
	 * use this information to determine what plug-in versions to install.
	 * 
	 * @return the canonical host type
	 */ 
	static const char* host_type() throw ();

	/**
	 * Sets a global fatal error handler. The error handler
	 * is called if a fatal unrecoverable error occurs. The default error
	 * handler prints the error message to standard error and aborts the
	 * program. This function should be called before creating a framework
	 * object to catch all fatal errors. It is not thread-safe with regards
	 * to other threads simultaneously invoking API.
	 * 
	 * @param feh the fatal error handler to be installed
	 */ 
	static void fatal_error_handler(::cpluff::fatal_error_handler &feh) throw ();

	/**
	 * Resets the default fatal error handler which prints the error message to
	 * standard error and aborts the program. This function is not thread-safe
	 * with regards to other threads simultaneously invoking API.
	 */
	static void reset_fatal_error_handler() throw ();
	
	/**
	 * Initializes the C-Pluff framework. The framework should be destroyed
	 * using cpluff::framework::destroy when framework services are not needed
	 * anymore. This function can be called several times. Global
	 * initialization occurs at the first call when in uninitialized state.
	 * This function is not thread-safe with
	 * regards to other threads simultaneously initializing or destroying the
	 * framework.
	 * 
	 * Additionally, to enable localization support, the main program should
	 * set the current locale using @code setlocale(LC_ALL, "") @endcode
	 * before calling this function for the first time.
	 * 
	 * @throw api_error if there are not enough system resources
	 */ 
	static void init() throw (api_error);

	/**
	 * Destroys the framework. All plug-in containers are destroyed and all
	 * references and pointers obtained from the plug-in framework become
	 * invalid. Framework is destroyed when this function has been
	 * called as many times as cpluff::framework::init.
	 * This function is not thread-safe with regards to other threads
	 * simultaneously initializing or destroying the framework.
	 */
	static void destroy();

	/**
	 * Creates and returns a new plug-in container. The returned plug-in
	 * container should be destroyed by calling its destroy method when
	 * it is not needed anymore. Any remaining plug-in containers are
	 * destroyed when the framework is destroyed.
	 *
	 * @return reference to a new created plug-in container
	 * @throw api_error if there are not enough system resources
	 */
	static plugin_container &new_plugin_container() throw (api_error);

private:

	/** @internal */
	inline framework() {};

	/** @internal */
	inline ~framework() {};
};

/**
 * A plug-in context represents the co-operation environment of a set of
 * plug-ins from the perspective of a particular participating plug-in or
 * the perspective of the main program. It is used to access
 * the shared resources but the framework also uses the context to identify
 * the plug-in or the main program invoking framework functions. Therefore
 * a plug-in should not generally expose its context object to other
 * plug-ins or the main program and neither should the main program
 * expose its context object to plug-ins. Main program creates plug-in contexts
 * using CPFramework::createPluginContainer. Plug-ins receive a reference to
 * the associated plug-in context via CPPluginImplementation::getContext.
 */
class plugin_context {
public:

	/**
	 * Registers a logger with this plug-in context or updates the settings of
	 * a registered logger. The logger will receive selected log messages.
	 * If the specified logger is not yet known, a new logger registration
	 * is made, otherwise the settings for the existing logger are updated.
	 * The logger can be unregistered using cpluff::unregister_logger and it is
	 * automatically unregistered when the registering plug-in is stopped or
	 * when the context is destroyed.
	 *
	 * @param logger the logger object to be registered
	 * @param minseverity the minimum severity of messages passed to logger
	 * @throw cpluff::api_error if insufficient memory
	 * @sa cpluff::unregister_logger
	 */
	virtual void register_logger(logger& logger, logger::severity minseverity) throw (api_error) = 0;

	/**
	 * Removes a logger registration.
	 *
	 * @param logger the logger object to be unregistered
	 * @sa cpluff::register_logger
	 */
	virtual void unregister_logger(logger& logger) throw () = 0;

	/**
	 * Emits a new log message.
	 * 
	 * @param severity the severity of the event
	 * @param msg the log message (possibly localized)
	 */
	virtual void log(logger::severity severity, const char* msg) throw () = 0;

	/**
	 * Returns whether a message of the specified severity would get logged.
	 * 
	 * @param severity the target logging severity
	 * @return whether a message of the specified severity would get logged
	 */
	virtual bool is_logged(logger::severity severity) throw () = 0;

protected:

	/** @internal */
	inline ~plugin_context() {};
};

/**
 * A plug-in container is a container for plug-ins. It represents plug-in
 * context from the view point of the main program.
 */
class plugin_container : public virtual plugin_context {
public:

	/**
	 * Registers a plug-in collection with this container. A plug-in collection
	 * is a directory that has plug-ins as its immediate subdirectories. The
	 * directory is scanned for plug-ins when @ref scanPlugins is called.
	 * A plug-in collection can be unregistered using @ref unregister_plugin_collection or
	 * @ref unregister_plugin_collections. The specified directory path is
	 * copied.
	 * 
	 * @param dir the directory
	 * @throw api_error if insufficient memory
	 * @sa unregister_plugin_collection
	 * @sa unregister_plugin_collections
	 */
	virtual void register_plugin_collection(const char* dir) throw (api_error) = 0;

	/**
	 * Unregisters a plug-in collection previously registered with this
	 * plug-in container. Plug-ins already loaded from the collection are not
	 * affected. Does nothing if the directory has not been registered.
	 * 
	 * @param dir the previously registered directory
	 * @sa register_plugin_collection
	 */
	virtual void unregister_plugin_collection(const char* dir) throw () = 0;

	/**
	 * Unregisters all plug-in collections registered with this plug-in
	 * container. Plug-ins already loaded from collections are not affected.
	 * 
	 * @sa register_plugin_collection
	 */
	virtual void unregister_plugin_collections() throw () = 0;

	/**
	 * Loads a plug-in descriptor from the specified plug-in installation
	 * path and returns information about the plug-in. The plug-in descriptor
	 * is validated during loading. Possible loading errors are logged via this
	 * plug-in container. The plug-in is not installed to the container.
	 * The caller must release the returned information by calling
	 * plugin_info::release when it does not need the information
	 * anymore, typically after installing the plug-in.
	 * 
	 * @param path the installation path of the plug-in
	 * @return reference to the plug-in information structure
	 * @throw cp_api_error if loading fails or the plug-in descriptor is malformed
	 */
	virtual plugin_info& load_plugin_descriptor(const char* path) throw (api_error) = 0;

	/**
	 * Destroys this plug-in container and releases the associated resources.
	 * Stops and uninstalls all plug-ins in the container. The container must
	 * not be accessed after calling this function. All pointers and references
	 * obtained via the container become invalid after call to destroy.
	 */
	virtual void destroy() throw () = 0;

protected:

	/** @internal */
	inline ~plugin_container() {};
};


}

#endif /*CPLUFFXX_H_*/
