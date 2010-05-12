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
 * Declares base classes for callback handlers.
 */

#ifndef CPLUFFXX_CALLBACKS_H_
#define CPLUFFXX_CALLBACKS_H_

#include <cpluffxx/enums.h>

namespace cpluff {

class CPPlugin;

/**
 * An abstract base class for a fatal error handler. The client program may
 * implement a concrete subclass and register it using
 * CPFramework::setFatalErrorHandler to use an application specific
 * error handler for fatal C-Pluff errors.
 */ 
class fatal_error_handler {

public:

	/**
	 * Handles a fatal error in an application specific way. C-Pluff
	 * API functions must not be called from within a fatal
	 * error handler invocation. If this function returns then the framework
	 * aborts the program.
	 * 
	 * @param msg the error message (possibly localized)
	 */
	virtual void fatal_error(const char* msg) = 0;
	
	virtual ~fatal_error_handler() = 0;
};

/**
 * An abstract base class for a framework logger. The client program may
 * implement a concrete subclass and register it using
 * CPFramework::registerLogger. There can be several registered loggers.
 */
class logger {

public:

	/**
	 * An enumeration of possible message severities for logger. These are also
	 * used when setting a minimum logging level in call to
	 * CPFramework::registerLogger.
	 */
	enum severity {

		/**
		 * Used for detailed debug messages. This level of logging is enabled
		 * only if debugging has been enabled at framework compilation time.
		 */
		DEBUG,
	
		/** Used for informational messages such as plug-in state changes */
		INFO,
	
		/** Used for messages warning about possible problems */
		WARNING,
	
		/** Used for messages reporting plug-in framework errors */
		ERROR
	
	};

	/**
	 * Logs a framework event or an error. The
	 * messages may be localized. Plug-in framework API functions must not
	 * be called from within a logger function invocation. Logger function
	 * invocations associated with the same framework instance are
	 * serialized if the framework is compiled with multi-threading
	 * support. Loggers are registered using CPFramework::addLogger.
	 * 
	 * @param sev severity of the event
	 * @param msg a possibly localized log message
	 * @param apid identifier of the action initiating plug-in or NULL for the client program
	 */
	virtual void log(severity sev, const char* msg, const char* apid) = 0;
	
	virtual ~logger() = 0;
};

/**
 * An abstract base class for a plug-in listener. The client program may
 * implement a concrete subclass and register it using
 * CPPluginContext::registerPluginListener to receive information about
 * plug-in state changes.
 */
class plugin_listener {

public:

	/**
	 * Called synchronously after a plugin state change.
	 * The function should return promptly.
	 * Framework initialization, plug-in context management, plug-in management,
	 * listener registration and dynamic symbol functions must not be called
	 * from within a plug-in listener invocation.
	 * 
	 * @param plugin the changed plug-in
	 * @param old_state the old plug-in state
	 * @param new_state the new plug-in state
	 */	
	virtual void plugin_state_change(const CPPlugin& plugin, plugin_state old_state, plugin_state new_state) = 0;

	virtual ~plugin_listener() = 0;
};

}

#endif /*CPLUFFXX_CALLBACKS_H_*/
