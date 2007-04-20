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
 * The elements declared here constitute the C-Pluff C++ API. To use the
 * API include this file and link the main program and plug-in runtime
 * libraries with the C-Pluff C++ library. In addition to local declarations,
 * this file also includes cpluffdef.h header file for defines common to C
 * and C++ API.
 * 
 * At the moment this API is work in progress and changes are likely.
 */

#ifndef CPLUFFXX_H_
#define CPLUFFXX_H_

#include <vector>
#include <map>
#include <cstddef>

/**
 * @defgroup cxxDefines Defines
 * Preprocessor defines.
 */

#include <cpluffdef.h>


/* ------------------------------------------------------------------------
 * Defines
 * ----------------------------------------------------------------------*/

/**
 * @def CP_CXX_API
 * @ingroup cxxDefines
 *
 * Marks a symbol declaration to be part of the C-Pluff C++ API.
 * This macro declares the symbol to be imported from the C-Pluff library.
 */

#ifndef CP_CXX_API
#define CP_CXX_API CP_IMPORT
#endif

namespace org {
namespace cpluff {


/* ------------------------------------------------------------------------
 * Enumerations
 * ----------------------------------------------------------------------*/

/**
 * @defgroup cxxEnums Enumerations
 * Constant value enumerations.
 */
//@{

/**
 * An enumeration of possible plug-in states. Plug-in states can be
 * observed by registering a CPPluginListener using
 * CPPluginContext::registerPluginListener or by calling CPPlugin::getState.
 *
 * @sa CPPluginListener
 * @sa CPPluginContext::getPluginState
 */
enum CPPluginState {

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

//@}


/* ------------------------------------------------------------------------
 * Classes
 * ----------------------------------------------------------------------*/

class CPPlugin;
class CPPluginContext;
class CPPluginContainer;
class CPPluginImport;
class CPExtensionPointDescriptor;
class CPExtensionDescriptor;
class CPConfigurationElement;

/**
 * @defgroup cxxClasses Classes
 * Public API classes. If the framework has been compiled with multi-threading
 * support then API functions are generally thread-safe with exceptions noted
 * in the API documentation.
 */

/**
 * @defgroup cxxClassesCallback Callback interfaces
 * @ingroup cxxClasses
 * Abstract callback interfaces to be implemented by the client program.
 * Concrete implementations of these classes can be registered as callbacks.
 */
//@{


/**
 * An abstract base class for a plug-in listener. The client program may
 * implement a concrete subclass and register it using
 * CPPluginContext::registerPluginListener to receive information about
 * plug-in state changes.
 */
class CPPluginListener {

public:

	/**
	 * Called synchronously after a plugin state change.
	 * The function should return promptly.
	 * Framework initialization, plug-in context management, plug-in management,
	 * listener registration and dynamic symbol functions must not be called
	 * from within a plug-in listener invocation.
	 * 
	 * @param plugin the changed plug-in
	 * @param oldState the old plug-in state
	 * @param newState the new plug-in state
	 */	
	CP_CXX_API virtual void pluginStateChanged(const CPPlugin& plugin, CPPluginState oldState, CPPluginState newState) = 0;

};

/**
 * An abstract base class for a fatal error handler. The client program may
 * implement a concrete subclass and register it using
 * CPFramework::setFatalErrorHandler to use an application specific
 * error handler for fatal C-Pluff errors.
 */ 
class CPFatalErrorHandler {

public:

	/**
	 * Handles a fatal error in an application specific way. C-Pluff
	 * API functions must not be called from within a fatal
	 * error handler invocation. If this function returns then the framework
	 * aborts the program.
	 * 
	 * @param msg the error message (possibly localized)
	 */
	CP_CXX_API virtual void handleFatalError(const char* msg) = 0;
};

/**
 * An abstract base class for a framework logger. The client program may
 * implement a concrete subclass and register it using
 * CPFramework::registerLogger. There can be several registered loggers.
 */
class CPLogger {

public:

	/**
	 * An enumeration of possible message severities for CPLogger. These are also
	 * used when setting a minimum logging level in call to
	 * CPFramework::registerLogger.
	 */
	enum Severity {

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
	 * @param severity severity of the event
	 * @param msg a possibly localized log message
	 * @param apid identifier of the action initiating plug-in or NULL for the client program
	 */
	CP_CXX_API virtual void log(Severity severity, const char* msg, const char* apid) = 0;
};

//@}


/**
 * @defgroup cxxClassesExceptions Exceptions
 * @ingroup cxxClasses
 * Exceptions thrown by API calls.
 */
//@{

/**
 * Thrown to indicate an error in a C-Pluff API call. API functions that may
 * fail throw this exception on error conditions. This class is not intended
 * to be subclassed by the client program.
 */
class CPAPIError {
public:

	/** Error codes included in CPAPIError. */
	enum ErrorCode {
		
		/** Not enough memory or other operating system resources available */
		RESOURCE = 1,

		/** The specified object is unknown to the framework */
		NKNOWN,

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
	 * Returns an error code describing the type of error. This may be
	 * used by the client program to determine the general nature of error.
	 * 
	 * @return an error code describing the type of error
	 */ 
	CP_CXX_API virtual ErrorCode getErrorCode() const throw () = 0;

	/**
	 * Returns an error message intended for display purposes. The error
	 * message may be localized. The error message becomes invalid when
	 * the exception is deleted. The returned reference is valid until this
	 * exception object is destructed.
	 * 
	 * @return a possibly localized error message intended for display purposes
	 */ 
	CP_CXX_API virtual const char* getErrorMessage() const throw () = 0;

	/**
	 * Destructs an exception releasing associated resources.
	 */
	CP_CXX_API virtual ~CPAPIError() throw () = 0;

};

//@}


/**
 * @defgroup cxxClassesData Data encapsulation classes
 * @ingroup cxxClasses
 * Classes encapsulating framework data.
 */
//@{


/**
 * A base class for reference counted objects. These objects are created on
 * demand by the framework and they are deleted once the last reference to the
 * object is released. This class is not intended to be subclassed by the
 * client program.
 */
class CPReferenceCounted {
public:

	/**
	 * Increases the reference count of this object. This member function
	 * can be used if the reference is to be passed on to another part of
	 * the client program that will release its reference independently from
	 * the code that is passing the reference.
	 */ 
	CP_CXX_API virtual void use() throw () = 0;
	
	/**
	 * Releases a reference to this object and automatically destructs the
	 * object if reference count drops to zero. The reference must not be used
	 * after calling this member function.
	 * 
	 * @internal
	 * Deletes this object when the reference count drops to zero.
	 */
	CP_CXX_API virtual void release() throw () = 0;

protected:

	CP_HIDDEN ~CPReferenceCounted() {};

};

/**
 * A generic convenience container for a reference to a reference counted
 * object. This class can be used to ensure that the reference counted object
 * is automatically released when it is not needed anymore. To ensure this,
 * do not maintain direct pointers or references to the reference counted
 * object but instead use instances of this class with automatic storage
 * duration and access the object via them. This class is not thread-safe with
 * regards to other threads accessing the same object instance.
 * 
 * Here is a simple example of the use of this class.
 * 
 * @code
 * #include <iostream>
 * #include <cpluffxx.h>
 * 
 * using namespace std;
 * using namespace org::cpluff;
 * 
 * CPRef<CPPluginDescriptor> installPlugin(CPContext& ctx, const char* path) {
 *   CPRef<CPPluginDescriptor> pd = ctx.loadPluginDescriptor(path);
 *   ctx.installPlugin(*pd);
 *   return pd;
 *
 *   // The descriptor is automatically released here unless the caller stores
 *   // the returned copy of CPRef for further processing.
 * }
 * 
 * void installPlugins(CPContext& ctx, vector<const char*>& paths) {
 *   vector<const char*>::iterator iter;
 *   for (iter = paths.begin(); iter != paths.end(); iter++) {
 *     CPRef<CPPluginDescriptor> pd = installPlugin(ctx, *iter);
 *     cout << "Installed plug-in " << (*pd).getName() << newline;
 * 
 *     // The descriptor received from installPlugin is released here
 *     // automatically when local reference container is destructed.
 *   }
 * }
 * @endcode
 */
template <class T> class CPRef {
public:

	/**
	 * Constructs a new empty container. It can be initialized by assigning
	 * an initialized container or a reference counted object to it.
	 */
	inline CPRef(): object(NULL) {}

	/**
	 * Constructs and initializes a contained reference from supplied object
	 * reference.
	 * 
	 * @oaram o reference to a reference counted object
	 */
	inline CPRef(T& o): object(&o) {}

	/**
	 * Constructs and initializes a contained reference from supplied object
	 * pointer.
	 * 
	 * @param o pointer to a reference counted object
	 */
	inline CPRef(T* o): object(o) {}

	/**
	 * Constructs and initializes a contained reference from another contained
	 * reference. Increments the reference count of the contained object, if
	 * any.
	 * 
	 * @param r contained reference to be copied
	 */
	inline CPRef(const CPRef& r): object(r.object) {
		if (object != NULL) {
			static_cast<CPReferenceCounted*>(object)->use();
		}
	}

	/**
	 * Destructs a contained reference. Decrements the reference count of the
	 * contained object, if any.
	 */
	inline ~CPRef() {
		if (object != NULL) {
			static_cast<CPReferenceCounted*>(object)->release();
		}
	}

	/**
	 * Returns the contained reference.
	 * 
	 * @return the contained reference
	 */
	inline T& operator*() const {
		return *object;
	}

	/**
	 * Assigns the reference in the specified container to this container.
	 * Decrements the reference count of the previously contained object,
	 * if any, and increments the reference count of the new object,
	 * if any.
	 * 
	 * @param r contained reference to be assigned
	 */
	inline CPRef& operator=(const CPRef& r) {
		if (r.object != NULL) {
			static_cast<CPReferenceCounted*>(r.object)->use();
		}
		if (object != NULL) {
			static_cast<CPReferenceCounted*>(object)->release();
		}
		object = r.object;
	}

	/**
	 * Assigns the specified reference to this container.
	 * Decrements the reference count of the previously contained object,
	 * if any. The specified reference must not be contained in any CPRef
	 * object.
	 *
	 * @param o the reference to be assigned
	 */ 
	inline CPRef& operator=(T& o) {
		if (object != NULL) {
			static_cast<CPReferenceCounted*>(object)->release();
		}
		object = &o;
	}
	
	/**
	 * Assigns the specified pointer to this container.
	 * Decrements the reference count of the previously contained object,
	 * if any. The specified pointer must not be contained in any CPRef
	 * object.
	 *
	 * @param o the pointer to be assigned
	 */ 
	inline CPRef& operator=(T* o) {
		if (object != NULL) {
			static_cast<CPReferenceCounted*>(object)->release();
		}
		object = o;
	}

protected:

	/** A pointer to the encapsuled reference counted object */
	T* object;
};

/**
 * A generic collection of objects of type T. This interface is used instead
 * of STL classes to hide implementation details. This class is not intended
 * to be subclassed by the client program.
 */
template <class T> class CPCollection {
public:

	/**
	 * Returns the number of objects in this collection.
	 * 
	 * @return the number of objects in this collection
	 */
	CP_CXX_API int virtual size() const throw () = 0;
	
	/**
	 * Returns whether this collection is empty or not.
	 * 
	 * @return whether this collection is empty or not
	 */
	CP_CXX_API bool isEmpty() const throw () = 0;
	
	/**
	 * Returns an iterator over the contained objects.
	 * 
	 * @return an iterator over the contained objects
	 */
	//CP_CXX_API Iterator<T> iterator() 
};

/**
 * Describes a plug-in to the framework and to other components.
 * This information can be loaded from a plug-in descriptor file using
 * CPPluginContext::loadPluginDescriptor. Corresponding information about
 * installed plug-ins can be obtained by using CPPluginContext::getPlugin
 * and CPPluginContext::getPlugins. This class corresponds to the top level
 * @a plugin element in a plug-in descriptor file. All references and pointers
 * obtained via this class become invalid after call to
 * CPReferenceCounted::release. This class is not intended to be subclassed
 * by the client program.
 */
class CPPluginDescriptor: public virtual CPReferenceCounted {
public:

	/**
	 * Returns the unique identifier of the plugin. A recommended way
	 * to generate identifiers is to use domain name service (DNS) prefixes
	 * (for example, org.cpluff.ExamplePlugin) to avoid naming conflicts. This
	 * corresponds to the @a id attribute of the @a plugin element in a plug-in
	 * descriptor.
	 * 
	 * @return the unique identifier of the plug-in
	 */	
	CP_CXX_API virtual const char* getIdentifier() const throw () = 0;

	/**
	 * Returns an optional plug-in name or NULL if the plug-in
	 * has no name. The value may be localized.
	 * 
	 * @return an optional plug-in name or NULL
	 */
	CP_CXX_API virtual const char* getName() const throw () = 0;

	/**
	 * Returns an optional plug-in version string or NULL if
	 * no version string is available.
	 * 
	 * @return an optional plug-in version string or NULL
	 */
	CP_CXX_API virtual const char* getVersion() const throw() = 0;
	
	/**
	 * Returns an optional provider name or NULL if provider
	 * name is not available. The value may be localized.
	 * 
	 * @return an optional provider name or NULL
	 */
	CP_CXX_API virtual const char* getProviderName() const throw () = 0;
	
	/**
	 * Returns the plug-in directory path or NULL if not known.
	 * 
	 * @return the plug-in directory path or NULL
	 */
	CP_CXX_API virtual const char* getPath() const throw () = 0;
	
	/**
	 * Returns optional ABI compatibility information or NULL
	 * if no information is available. The returned value is the earliest
	 * version of the plug-in interface the current interface is backwards
	 * compatible with when it comes to the application binary interface (ABI)
	 * of the plug-in. That is, plug-in clients compiled against any plug-in
	 * interface version from the returned version to the version returned by
	 * @ref getVersion (inclusive) can use the current version of the plug-in
	 * binary. This describes binary or runtime compatibility.
	 * The value corresponds to the @a abi-compatibility
	 * attribute of the @a backwards-compatibility element in a plug-in
	 * descriptor.
	 * 
	 * @return ABI compatibility information or NULL
	 */	
	CP_CXX_API virtual const char* getABIBackwardsCompatibility() const throw () = 0;

	/**
	 * Returns optional API compatibility information or NULL
	 * if no information is available. The returned value is the earliest
	 * version of the plug-in interface the current interface is backwards
	 * compatible with when it comes to the application programming interface
	 * (API) of the plug-in. That is, plug-in clients written for any plug-in
	 * interface version from @a api_bw_compatibility to @ref version
	 * (inclusive) can be compiled against the current version of the plug-in
	 * API. This describes source or build time compatibility. The value
	 * corresponds to the @a api-compatibility attribute of the
	 * @a backwards-compatibility element in a plug-in descriptor.
	 * 
	 * @return API compatibility information or NULL 
	 */
	CP_CXX_API virtual const char* getAPIBackwardsCompatibility() const throw () = 0;
	
	/**
	 * Returns an optional C-Pluff version requirement or NULL if no
	 * information is available. The returned value is the version of the
	 * C-Pluff implementation the plug-in was compiled against. It is used to
	 * determine the compatibility of the plug-in runtime and the linked in
	 * C-Pluff implementation. Any C-Pluff version that is backwards compatible
	 * on binary level with the specified version fulfills the requirement.
	 * 
	 * @return C-Pluff version requirement or NULL
	 */
	CP_CXX_API virtual const char* getRequiredCPluffVersion() const throw () = 0;

	/**
	 * Returns plug-in imports as a vector.
	 * 
	 * @return plug-in imports as a vector
	 */
	CP_CXX_API virtual const std::vector<const CPPluginImport*>& getImports() const throw () = 0;

    /**
     * Returns the base name of the plug-in runtime library or NULL
     * if none. A platform specific prefix (for example, "lib") and an extension
     * (for example, ".dll" or ".so") may be added to the base name.
     * The returned value corresponds to the @a library attribute of the
     * @a runtime element in a plug-in descriptor.
     * 
     * @return the base name of the plug-in runtime library or NULL
     */
	CP_CXX_API  virtual const char* getRuntimeLibraryName() const throw () = 0;

    /**
     * Returns the name of symbol pointing to the plug-in runtime function
     * information or NULL if none. The symbol with this name should
     * point to an instance of @ref cp_plugin_runtime_t structure. This
     * corresponds to the @a funcs attribute of the @a runtime element in a
     * plug-in descriptor.
     * 
     * @return the name of the symbol pointing to the plug-in runtime information or NULL
     */
	CP_CXX_API virtual const char* getRuntimeFunctionsSymbol() const throw () = 0;

	/**
	 * Returns the extension points provided by this plug-in.
	 * 
	 * @return extension points provided by this plug-in
	 */
	CP_CXX_API virtual const std::vector<const CPExtensionPointDescriptor*>& getExtensionPoints() const throw () = 0;
	
	/**
	 * Returns the extensions provided by this plug-in.
	 * 
	 * @return extensions provided by this plug-in
	 */
	CP_CXX_API virtual const std::vector<const CPExtensionDescriptor*>& getExtensions() const throw () = 0;

};

/**
 * Describes plugins dependency to other plug-ins. Import information can be
 * obtained using CPPluginDescriptor::getImports. This class is not intended
 * to be subclassed by the client program.
 */
class CPPluginImport {
public:

	/**
	 * Returns the identifier of the imported plug-in. This corresponds to the
	 * @a plugin attribute of the @a import element in a plug-in descriptor.
	 * 
	 * @return the identifier of the imported plug-in
	 */
	CP_CXX_API virtual const char* getPluginIdentifier() const throw () = 0;
	
	/**
	 * Returns an optional version requirement or NULL if no version
	 * requirement. This is the version of the imported plug-in the importing
	 * plug-in was compiled against. Any version of the imported plug-in that is
	 * backwards compatible with this version fulfills the requirement.
	 * This corresponds to the @a if-version attribute of the @a import
	 * element in a plug-in descriptor.
	 * 
	 * @return an optional version requirement or NULL
	 */
	CP_CXX_API virtual const char* getVersion() const throw () = 0;

	/**
	 * Returns whether this import is optional.
	 * An optional import causes the imported plug-in to be started if it is
	 * available but does not stop the importing plug-in from starting if the
	 * imported plug-in is not available. If the imported plug-in is available
	 * but the API version conflicts with the API version requirement then the
	 * importing plug-in fails to start. This corresponds to the @a optional
	 * attribute of the @a import element in a plug-in descriptor.
	 * 
	 * @return whether this import is optional
	 */
	CP_CXX_API virtual bool isOptional() const throw () = 0;
};

/**
 * Describes a point of extensibility into which other plug-ins can install
 * extensions. Extension point information can be obtained by using
 * CPPluginDescriptor::getExtensionPoints. This class is not intended to
 * be subclassed by the client program.
 */
class CPExtensionPointDescriptor {
public:

	/**
	 * Returns the local identifier uniquely identifying the extension point
	 * within the host plug-in. This corresponds to the @name id attribute of
	 * an @a extension-point element in a plug-in descriptor.
	 * 
	 * @return the local identifier of the extension point
	 */
	CP_CXX_API virtual const char* getLocalIdentifier() const throw () = 0;
	
	/**
	 * Returns the unique identifier of the extension point. This is
	 * automatically constructed by concatenating the identifier of the host
	 * plug-in and the local identifier of the extension point.
	 * 
	 * @return the unique identifier of the extension point
	 */
	CP_CXX_API virtual const char* getIdentifier() const throw () = 0;
	
	/**
	 * Returns an optional extension point name or NULL. The
	 * extension point name is intended for display purposes only and the value
	 * can be localized. This corresponds to the @a name attribute of
	 * an @a extension-point element in a plug-in descriptor.
	 * 
	 * @return an optional extension point name or NULL
	 */
	CP_CXX_API virtual const char* getName() const throw () = 0;
	
	/**
	 * Returns an optional path to the extension schema definition or NULL
	 * if not available. The path is relative to the plug-in directory.
	 * This corresponds to the @a schema attribute
	 * of an @a extension-point element in a plug-in descriptor.
	 * 
	 * @return an optional path to the extension schema defition or NULL
	 */
	CP_CXX_API virtual const char* getSchemaPath() const throw () = 0;
	
};

/**
 * Describes an extension attached to an extension point. Extension information
 * can be obtained by using CPPluginDescriptor::getExtensions. This class is
 * not intended to be subclassed by the client program.
 */
class CPExtensionDescriptor {
public:
	
	/**
	 * Returns the unique identifier of the extension point this extension is
	 * attached to. This corresponds to the @a point attribute of an
	 * @a extension element in a plug-in descriptor.
	 * 
	 * @return the unique identifier of the associated extension point
	 */
	CP_CXX_API virtual const char* getExtensionPointIdentifier() const throw () = 0;
	
	/**
	 * Returns an optional local identifier uniquely identifying the extension
	 * within the host plug-in or NULL if no available. This
	 * corresponds to the @a id attribute of an @a extension element in a
	 * plug-in descriptor.
	 * 
	 * @returns an optional local identifier of the extension or NULL
	 */
	CP_CXX_API virtual const char* getLocalIdentifier() const throw () = 0;
	
    /**
     * Returns an optional unique identifier of the extension or NULL
     * if not available. This is automatically constructed by
     * concatenating the identifier of the host plug-in and the local
     * identifier of the extension.
     * 
     * @return an optional unique identifier of the extension or NULL
     */
    CP_CXX_API virtual const char* getIdentifier() const throw () = 0;

	/** 
	 * Returns an optional extension name or NULL if not available.
	 * The extension name is intended for display purposes only and the value
	 * can be localized. This corresponds to the @a name attribute of an
	 * @a extension element in a plug-in descriptor.
	 * 
	 * @return an optional extension name or NULL
	 */
	CP_CXX_API virtual const char* getName() const throw () = 0;

	/**
	 * Returns extension configuration starting with the extension element.
	 * This includes extension configuration information as a tree of
	 * configuration elements. These correspond to the @a extension
	 * element and its contents in a plug-in descriptor.
	 * 
	 * @return extension configuration starting with the extension element
	 */
	CP_CXX_API virtual const CPConfigurationElement& getConfiguration() const throw () = 0;

};

/**
 * Contains configuration information for an extension. The root configuration
 * element is available from CPExtensionDescriptor::getConfiguration and
 * descendant elements can be accessed via their ancestors. The actual
 * semantics of the configuration information are defined by the associated
 * extension point. This class is not intended to be subclassed by the client
 * program.
 */
class CPConfigurationElement {
public:

	/**
	 * Returns the name of the configuration element. This corresponds to the
	 * name of the element in a plug-in descriptor.
	 * 
	 * @return the name of the configuration element
	 */
	CP_CXX_API virtual const char* getName() const throw () = 0;

	/**
	 * Returns the attribute name, value map for this element. This corresponds
	 * to the attributes of the element.
	 * 
	 * @return the attribute map for this element
	 */
	CP_CXX_API virtual const std::map<const char*, const char*>& getAttributes() const throw () = 0;

	/**
	 * Returns a pointer to the parent element or NULL if this is a root
	 * element.
	 * 
	 * @return a pointer to the parent element or NULL if this is the root
	 */
	CP_CXX_API virtual const CPConfigurationElement* getParent() const throw () = 0;
	
	/**
	 * Returns the children of this configuration element as a vector.
	 * 
	 * @return the children of this configuration element as a vector
	 */
	CP_CXX_API virtual const std::vector<CPConfigurationElement>& getChildren() const throw () = 0;

};

//@}


/**
 * @defgroup cxxClassesAPI Functional API classes
 * @ingroup cxxClasses
 * Public classes providing the API functionality.
 */
//@{

/**
 * The core class used for global initialization and to access framework
 * functionality. It also provides static information about the framework
 * implementation. This class is not intended to be subclassed by the client
 * program.
 */
class CPFramework {
public:

	/**
	 * Returns the release version string of the linked in C-Pluff
	 * implementation.
	 * 
	 * @return the release version of the C-Pluff implementation 
	 */
	CP_CXX_API static const char* getVersion() throw ();

	/**
	 * Returns the canonical host type associated with the linked in
	 * C-Pluff implementation. A multi-platform installation manager could
	 * use this information to determine what plug-in versions to install.
	 * 
	 * @return the canonical host type
	 */ 
	CP_CXX_API static const char* getHostType() throw ();

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
	CP_CXX_API static void setFatalErrorHandler(CPFatalErrorHandler &feh) throw ();

	/**
	 * Resets the default fatal error handler which prints the error message to
	 * standard error and aborts the program. This function is not thread-safe
	 * with regards to other threads simultaneously invoking API.
	 */
	CP_CXX_API static void resetFatalErrorHandler() throw ();
	
	/**
	 * Initializes the C-Pluff framework. The framework should be destroyed
	 * using CPFramework::destroy when framework services are not needed
	 * anymore. This function can be called several times and it returns
	 * distinct framework objects. Global initialization occurs at the first
	 * call when in uninitialized state. This function is not thread-safe with
	 * regards to other threads simultaneously initializing or destroying the
	 * framework.
	 * 
	 * Additionally, to enable localization support, the main program should
	 * set the current locale using @code setlocale(LC_ALL, "") @endcode
	 * before calling this function for the first time.
	 * 
	 * @exception CPAPIError if there is not enough system resources
	 */ 
	CP_CXX_API static CPFramework& init();

	/**
	 * Destroys the framework. All plug-in contexts created via this framework
	 * object are destroyed and all references and pointers obtained via this
	 * framework object become invalid. Global deinitialization occurs when
	 * all framework objects have been destroyed.
	 * This function is not thread-safe with regards to other threads
	 * simultaneously initializing or destroying the framework.
	 */
	CP_CXX_API virtual void destroy() throw () = 0;

	/**
	 * Creates a new plug-in container. Plug-ins are loaded and installed into
	 * a specific container. The main program may have more than one plug-in
	 * container but the plug-ins that interact with each other should be
	 * placed in the same container. The resources associated with the
	 * container are released by destroying the container via
	 * CPPluginContainer::destroy when it is not needed anymore.
	 * Remaining containers created via this framework object are automatically
	 * destroyed when the framework object is destroyed.
	 * 
	 * @return the newly created plugin container
	 * @throw CPAPIError if an error occurs
	 */
	CP_CXX_API virtual CPPluginContainer& createPluginContainer() = 0;

protected:

	CP_HIDDEN ~CPFramework() {};

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
class CPPluginContext {
public:

	/**
	 * Registers a logger with this plug-in context or updates the settings of
	 * a registered logger. The logger will receive selected log messages.
	 * If the specified logger is not yet known, a new logger registration
	 * is made, otherwise the settings for the existing logger are updated.
	 * The logger can be unregistered using @ref unregisterLogger and it is
	 * automatically unregistered when the registering plug-in is stopped or
	 * when the context is destroyed. 
	 *
	 * @param logger the logger object to be registered
	 * @param minSeverity the minimum severity of messages passed to logger
	 * @throw CPAPIError if insufficient memory
	 */
	CP_CXX_API virtual void registerLogger(CPLogger& logger, CPLogger::Severity minSeverity) = 0;

	/**
	 * Removes a logger registration.
	 *
	 * @param logger the logger object to be unregistered
	 * @sa registerLogger
	 */
	CP_CXX_API virtual void unregisterLogger(CPLogger& logger) throw () = 0;

	/**
	 * Emits a new log message.
	 * 
	 * @param severity the severity of the event
	 * @param msg the log message (possibly localized)
	 */
	CP_CXX_API virtual void log(CPLogger::Severity severity, const char* msg) = 0;

	/**
	 * Returns whether a message of the specified severity would get logged.
	 * 
	 * @param severity the target logging severity
	 * @return whether a message of the specified severity would get logged
	 */
	CP_CXX_API virtual bool isLogged(CPLogger::Severity severity) throw () = 0;

protected:

	CP_HIDDEN ~CPPluginContext() {};
};

/**
 * A plug-in container is a container for plug-ins. It represents plug-in
 * context from the view point of the main program.
 */
class CPPluginContainer : public virtual CPPluginContext {
public:

	/**
	 * Destroys this plug-in container and releases the associated resources.
	 * Stops and uninstalls all plug-ins in the container. The container must
	 * not be accessed after calling this function. All pointers and references
	 * obtained via the container become invalid after call to destroy.
	 */
	CP_CXX_API virtual void destroy() throw () = 0;

	/**
	 * Registers a plug-in collection with this container. A plug-in collection
	 * is a directory that has plug-ins as its immediate subdirectories. The
	 * directory is scanned for plug-ins when @ref scanPlugins is called.
	 * A plug-in collection can be unregistered using @ref unregisterPluginCollection or
	 * @ref unregisterPluginCollections. The specified directory path is
	 * copied.
	 * 
	 * @param dir the directory
	 * @throw CPAPIError if insufficient memory
	 */
	CP_CXX_API virtual void registerPluginCollection(const char* dir) = 0;

	/**
	 * Unregisters a plug-in collection previously registered with this
	 * plug-in container. Plug-ins already loaded from the collection are not
	 * affected. Does nothing if the directory has not been registered.
	 * 
	 * @param dir the previously registered directory
	 * @sa registerPluginCollection
	 */
	CP_CXX_API virtual void unregisterPluginCollection(const char* dir) throw () = 0;

	/**
	 * Unregisters all plug-in collections registered with this plug-in
	 * container. Plug-ins already loaded from collections are not affected.
	 * 
	 * @sa registerPluginCollection
	 */
	CP_CXX_API virtual void unregisterPluginCollections() throw () = 0;	

	/**
	 * Loads a plug-in descriptor from the specified plug-in installation
	 * path and returns information about the plug-in. The plug-in descriptor
	 * is validated during loading. Possible loading errors are logged via this
	 * plug-in container. The plug-in is not installed to the container.
	 * The caller must release the returned information by calling
	 * CPPluginDescriptor::release when it does not need the information
	 * anymore, typically after installing the plug-in.
	 * 
	 * @param path the installation path of the plug-in
	 * @return reference to the information structure
	 * @throw CPAPIError if loading fails or the plug-in descriptor is malformed
	 */
	CP_CXX_API virtual CPPluginDescriptor& loadPluginDescriptor(const char* path) = 0;

protected:

	CP_HIDDEN ~CPPluginContainer() {};
};

//@}


}}

#endif /*CPLUFFXX_H_*/
