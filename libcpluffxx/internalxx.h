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
#include <cpluffxx.h>
#include "../libcpluff/defines.h"
#include "../libcpluff/shared.h"
#include "util.h"

/* -----------------------------------------------------------------------
 * Implementation classes
 * ---------------------------------------------------------------------*/

namespace cpluff {

class plugin_context_impl;
class plugin_container_impl;
class plugin_import_impl;

class plugin_import_impl: public plugin_import {
public:

	/**
	 * Constructs a new plug-in import from a C API plug-in import.
	 * 
	 * @param pimport the C API plug-in import
	 */
	CP_HIDDEN plugin_import_impl(cp_plugin_import_t* pimport);

	CP_HIDDEN const char* get_plugin_identifier() const throw ();

	CP_HIDDEN const char* get_version() const throw ();

	CP_HIDDEN bool is_optional() const throw ();

private:

	/** The associated C API plug-in import */
	cp_plugin_import_t *pimport;
};

class plugin_context_impl : public virtual plugin_context {
public:

	/**
	 * Constructs a new plug-in context associated with the specified C API
	 * plug-in context handle.
	 * 
	 * @param context the C API plug-in context handle
	 */
	CP_HIDDEN plugin_context_impl(cp_context_t *context);

	CP_HIDDEN void register_logger(logger* logger, logger::severity minseverity) throw (api_error);

	CP_HIDDEN void unregister_logger(logger* logger) throw ();

	CP_HIDDEN void log(logger::severity severity, const char* msg) throw ();

	CP_HIDDEN bool is_logged(logger::severity severity) throw ();

	/**
	 * Returns the associated C API plug-in context handle.
	 * 
	 * @return the associated C API plug-in context handle
	 */
	CP_HIDDEN inline cp_context_t* getCContext() throw () {
		return context;
	}

	/**
	 * Emits a new formatted log message if the associated severity is being
	 * logged. Uses gettext to translate the message.
	 * 
	 * @param severity the severity of the event
	 * @param msg the log message (possibly localized)
	 */
	CP_HIDDEN void logf(logger::severity severity, const char* msg, ...) throw ();

protected:

	/**
	 * Handle to the underlying C API plug-in context.
	 */
	cp_context_t* context;

	/**
	 * Constructs a new unassociated plug-in context. The subclass constructor
	 * must set @ref context.
	 */
	CP_HIDDEN plugin_context_impl();

	/**
	 * Destructs a plug-in context. All resources associated with this
	 * plug-in context are released and all pointers and references
	 * obtained via it become invalid.
	 */
	CP_HIDDEN ~plugin_context_impl() throw ();

private:

	/**
	 * The registered loggers and their minimum logging severity.
	 */
	std::map<logger*, logger::severity> loggers;

	/**
	 * The minimum logging severity for registered loggers.
	 */
	logger::severity min_logger_severity;

	/**
	 * Delivers a logged message to all registered C++ loggers of a specific
	 * plug-in context object.
	 * 
	 * @param severity the severity of the message
	 * @param msg the message to be logged, possibly localized
	 * @param apid the identifier of the activating plug-in or NULL for the main program
	 * @param user_data pointer to the associated plug-in context object
	 */
	CP_HIDDEN static void deliver_log_message(cp_log_severity_t severity, const char* msg, const char* apid, void* user_data) throw ();

	/**
	 * Updates the aggregate minimum severity for installed loggers.
	 */
	CP_HIDDEN void update_min_logger_severity() throw (); 
};

class plugin_container_impl : public plugin_container, public plugin_context_impl {
public:

	/**
	 * Constructs a new plug-in container.
	 */
	CP_HIDDEN plugin_container_impl();
	
	CP_HIDDEN void register_plugin_collection(const char* dir) throw (api_error);

	CP_HIDDEN void unregister_plugin_collection(const char* dir) throw ();

	CP_HIDDEN void unregister_plugin_collections() throw ();

	CP_HIDDEN plugin_info& load_plugin_descriptor(const char* path) throw (api_error);

	CP_HIDDEN void destroy() throw ();

private:

	inline ~plugin_container_impl() {};

};

}

#endif /*INTERNALXX_H_*/
