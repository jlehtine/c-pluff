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
 * Enumerations for C-Pluff C++ API.
 */

#ifndef CPLUFFXX_ENUMS_H_
#define CPLUFFXX_ENUMS_H_

namespace cpluff {

/**
 * An enumeration of possible plug-in states. Plug-in states can be
 * observed by registering a %plugin_listener using
 * CPPluginContext::registerPluginListener or by calling CPPlugin::getState.
 *
 * @sa plugin_listener
 * @sa CPPluginContext::getPluginState
 */
enum plugin_state {

	/**
	 * Plug-in is not installed. No plug-in information has been
	 * loaded.
	 */
	CP_PLUGIN_UNINSTALLED,
	
	/**
	 * Plug-in is installed. At this stage the plug-in information has
	 * been loaded but its dependencies to other plug-ins has not yet
	 * been resolved. The plug-in runtime has not been loaded yet.
	 * The extension points and extensions provided by the plug-in
	 * have been registered.
	 */
	CP_PLUGIN_INSTALLED,
	
	/**
	 * Plug-in dependencies have been resolved. At this stage it has
	 * been verified that the dependencies of the plug-in are satisfied
	 * and the plug-in runtime has been loaded but it is not active
	 * (it has not been started or it has been stopped).
	 * Plug-in is resolved when a dependent plug-in is being
	 * resolved or before the plug-in is started. Plug-in is put
	 * back to installed stage if its dependencies are being
	 * uninstalled.
	 */
	CP_PLUGIN_RESOLVED,
	
	/**
	 * Plug-in is starting. The plug-in has been resolved and the start
	 * function (if any) of the plug-in runtime is about to be called.
	 * A plug-in is started when explicitly requested by the main
	 * program or when a dependent plug-in is about to be started or when
	 * a dynamic symbol defined by the plug-in is being resolved. This state
	 * is omitted and the state changes directly from resolved to active
	 * if the plug-in runtime does not define a start function.
	 */
	CP_PLUGIN_STARTING,
	
	/**
	 * Plug-in is stopping. The stop function (if any) of the plug-in
	 * runtime is about to be called. A plug-in is stopped if the start
	 * function fails or when stopping is explicitly
	 * requested by the main program or when its dependencies are being
	 * stopped. This state is omitted and the state changes directly from
	 * active to resolved if the plug-in runtime does not define a stop
	 * function.
	 */
	CP_PLUGIN_STOPPING,
	
	/**
	 * Plug-in has been successfully started and it has not yet been
	 * stopped.
	 */
	CP_PLUGIN_ACTIVE
	
};

}

#endif /*CPLUFFXX_ENUMS_H_*/
