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
 * Declares container classes for static plug-in information.
 */

#ifndef CPLUFFXX_INFO_H_
#define CPLUFFXX_INFO_H_

#include <cstring>
#include <vector>
#include <cpluff.h>
#include <cpluffxx/sharedptr.h>

namespace cpluff {

/**
 * Provides a comparison function for string pointer content.
 * This class is not intended to be used by the client program except as
 * part of C-Pluff containers using it.
 */
class less_str {
public:

	/*
	 * Returns whether the first string comes before the second string.
	 * 
	 * @param s1 the first string to compare
	 * @param s2 the second string to compare
	 * @return whether the first string comes before the second string
	 */
	CP_CXX_API inline bool operator()(const char* const& s1, const char* const& s2) {
		return strcmp(s1, s2) < 0;
	}
};

/**
 * Describes plugins dependency to other plug-ins. Import information can be
 * obtained using plugin_info::getImports. This class is not intended to be
 * instantiated or subclassed by the client program.
 */
class plugin_import {
public:

	/**
	 * @internal
	 * Constructs a new plug-in import and associates it with a C API
	 * plug-in import.
	 * 
	 * @param pdescriptor the associated plug-in descriptor
	 * @param pimport the associated C API plug-in import structure 
	 */
	CP_CXX_API inline plugin_import(const cp_plugin_import_t* pimport):
	pimport(pimport) {}

	/**
	 * Returns the identifier of the imported plug-in. This corresponds to the
	 * @a plugin attribute of the @a import element in a plug-in descriptor.
	 * 
	 * @return the identifier of the imported plug-in
	 */
	CP_CXX_API inline const char* plugin_id() const {
		return pimport->plugin_id;
	};
	
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
	CP_CXX_API inline const char* version() const {
		return pimport->version;
	};

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
	CP_CXX_API inline bool optional() const {
		return pimport->optional;
	};

protected:

	/** @internal The associated C API plug-in import */
	const cp_plugin_import_t* pimport;
};

/**
 * Describes a point of extensibility into which other plug-ins can install
 * extensions. Extension point information can be obtained by using
 * plugin_info::getExtensionPoints. This class is not intended to be
 * instantiated or subclassed by the client program.
 */
class ext_point_info {
public:

	/**
	 * @internal
	 * Constructs a new plug-in extension point descriptor and associates it
	 * with a C API extension point.
	 * 
	 * @param extpt the associated C API extension point structure
	 */
	CP_CXX_API inline ext_point_info(const cp_ext_point_t* extpt):
	extpt(extpt) {}

	/**
	 * Returns the local identifier uniquely identifying the extension point
	 * within the host plug-in. This corresponds to the @name id attribute of
	 * an @a extension-point element in a plug-in descriptor.
	 * 
	 * @return the local identifier of the extension point
	 */
	CP_CXX_API inline const char* local_id() const {
		return extpt->local_id;
	}
	
	/**
	 * Returns the unique identifier of the extension point. This is
	 * automatically constructed by concatenating the identifier of the host
	 * plug-in and the local identifier of the extension point.
	 * 
	 * @return the unique identifier of the extension point
	 */
	CP_CXX_API inline const char* identifier() const {
		return extpt->identifier;
	}
	
	/**
	 * Returns an optional extension point name or NULL. The
	 * extension point name is intended for display purposes only and the value
	 * can be localized. This corresponds to the @a name attribute of
	 * an @a extension-point element in a plug-in descriptor.
	 * 
	 * @return an optional extension point name or NULL
	 */
	CP_CXX_API inline const char* name() const {
		return extpt->name;
	}
	
	/**
	 * Returns an optional path to the extension schema definition or NULL
	 * if not available. The path is relative to the plug-in directory.
	 * This corresponds to the @a schema attribute
	 * of an @a extension-point element in a plug-in descriptor.
	 * 
	 * @return an optional path to the extension schema defition or NULL
	 */
	CP_CXX_API inline const char* schema_path() const {
		return extpt->schema_path;
	}
	
protected:

	/** @internal The associated C API extension point */
	const cp_ext_point_t* extpt;
};

/**
 * Contains configuration information for an extension. The root configuration
 * element is available from extension_info::getConfiguration and
 * descendant elements can be accessed via their ancestors. The actual
 * semantics of the configuration information are defined by the associated
 * extension point. This class is not intended to be subclassed by the client
 * program.
 */
class cfg_element {
public:

	/**
	 * @internal
	 * Constructs a new root configuration element associated with a
	 * C API configuration element structure.
	 * 
	 * @param cfge the associated C API configuration element
	 */
	CP_CXX_API inline cfg_element(const cp_cfg_element_t* cfge) {
	 	cfg_element(NULL, cfge);
	 }

	/**
	 * @internal
	 * Constructs a new configuration element associated with a 
	 * parent element and a C API configuration element structure.
	 * 
	 * @param parent the parent element or NULL if none
	 * @param cfge the associated C API configuration element
	 */
	CP_CXX_API cfg_element(cfg_element* parent, const cp_cfg_element_t* cfge);

	/**
	 * Returns the name of the configuration element. This corresponds to the
	 * name of the element in a plug-in descriptor.
	 * 
	 * @return the name of the configuration element
	 */
	CP_CXX_API inline const char* name() const {
		return cfge->name;
	}

	/**
	 * Returns the attribute name, value map for this element. This corresponds
	 * to the attributes of the element.
	 * 
	 * @return the attribute map for this element
	 */
	CP_CXX_API inline const std::map<const char*, const char*, less_str>& attributes() const {
		return attr_map;
	}

	/**
	 * Returns a pointer to the parent element or NULL if this is the root
	 * element.
	 * 
	 * @return a pointer to the parent element or NULL if root
	 */
	CP_CXX_API inline const cfg_element* parent() const {
		return cfg_parent;
	}
	
	/**
	 * Returns the children of this configuration element as a vector.
	 * 
	 * @return the children of this configuration element as a vector
	 */
	CP_CXX_API inline const std::vector<const cfg_element*>& children() const {
		return cfg_children;
	}

protected:

	/** @internal The associated C API configuration element */
	const cp_cfg_element_t* cfge;
	
	/** @internal The attribute map */
	std::map<const char*, const char*, less_str> attr_map;
	
	/** @internal The parent element or NULL */
	cfg_element* cfg_parent;
	
	/** @internal Children elements */
	std::vector<const cfg_element*> cfg_children;

};

/**
 * Describes an extension attached to an extension point. Extension information
 * can be obtained by using plugin_info::getExtensions. This class is not
 * intended to be instantiated or subclassed by the client program.
 */
class extension_info {
public:
	
	/**
	 * @internal
	 * Constructs a new plug-in extension descriptor and associates it
	 * with a C API extension.
	 * 
	 * @param ext the associated C API extension
	 */
	CP_CXX_API inline extension_info(const cp_extension_t* ext):
	ext(ext), cfg_root(ext->configuration) {}

	/**
	 * Returns the unique identifier of the extension point this extension is
	 * attached to. This corresponds to the @a point attribute of an
	 * @a extension element in a plug-in descriptor.
	 * 
	 * @return the unique identifier of the associated extension point
	 */
	CP_CXX_API inline const char* ext_point_id() const {
		return ext->ext_point_id;
	}
	
	/**
	 * Returns an optional local identifier uniquely identifying the extension
	 * within the host plug-in or NULL if no available. This
	 * corresponds to the @a id attribute of an @a extension element in a
	 * plug-in descriptor.
	 * 
	 * @returns an optional local identifier of the extension or NULL
	 */
	CP_CXX_API inline const char* local_id() const {
		return ext->local_id;
	}
	
    /**
     * Returns an optional unique identifier of the extension or NULL
     * if not available. This is automatically constructed by
     * concatenating the identifier of the host plug-in and the local
     * identifier of the extension.
     * 
     * @return an optional unique identifier of the extension or NULL
     */
    CP_CXX_API inline const char* identifier() const {
    	return ext->identifier;
    }

	/** 
	 * Returns an optional extension name or NULL if not available.
	 * The extension name is intended for display purposes only and the value
	 * can be localized. This corresponds to the @a name attribute of an
	 * @a extension element in a plug-in descriptor.
	 * 
	 * @return an optional extension name or NULL
	 */
	CP_CXX_API inline const char* name() const {
		return ext->name;
	}

	/**
	 * Returns extension configuration starting with the extension element.
	 * This includes extension configuration information as a tree of
	 * configuration elements. These correspond to the @a extension
	 * element and its contents in a plug-in descriptor.
	 * 
	 * @return extension configuration starting with the extension element
	 */
	CP_CXX_API inline const cfg_element& configuration() const {
		return cfg_root;
	}

protected:

	/** @internal The associated C APi extension */
	const cp_extension_t* ext;
	
	/** @internal The root configuration element */
	cfg_element cfg_root;
};

/**
 * Contains static plug-in information.
 * This information can be loaded from a plug-in descriptor file using
 * CPPluginContext::loadPluginDescriptor. Corresponding information about
 * installed plug-ins can be obtained by using CPPluginContext::getPlugin
 * and CPPluginContext::getPlugins. This class corresponds to the top level
 * @a plugin element in a plug-in descriptor file. This class is not intended
 * to be instantiated or subclassed by the client program.
 */
class plugin_info {
public:

	/**
	 * @internal
	 * Constructs a new plug-in descriptor and associates it with a C API
	 * plug-in descriptor.
	 * 
	 * @param context the associated C API plug-in context handle
	 * @param pinfo the associated C API plug-in descriptor
	 */
	CP_CXX_API plugin_info(cp_context_t* context, cp_plugin_info_t* pinfo);
	
	/**
	 * Returns the unique identifier of the plugin. A recommended way
	 * to generate identifiers is to use domain name service (DNS) prefixes
	 * (for example, org.cpluff.ExamplePlugin) to avoid naming conflicts. This
	 * corresponds to the @a id attribute of the @a plugin element in a plug-in
	 * descriptor.
	 * 
	 * @return the unique identifier of the plug-in
	 */	
	CP_CXX_API inline const char* identifier() const {
		return pinfo->identifier;
	}

	/**
	 * Returns an optional plug-in name or NULL if the plug-in
	 * has no name. The value may be localized.
	 * 
	 * @return an optional plug-in name or NULL
	 */
	CP_CXX_API inline const char* name() const {
		return pinfo->name;
	}

	/**
	 * Returns an optional plug-in version string or NULL if
	 * no version string is available.
	 * 
	 * @return an optional plug-in version string or NULL
	 */
	CP_CXX_API inline const char* version() const {
		return pinfo->version;
	}
	
	/**
	 * Returns an optional provider name or NULL if provider
	 * name is not available. The value may be localized.
	 * 
	 * @return an optional provider name or NULL
	 */
	CP_CXX_API inline const char* provider_name() const {
		return pinfo->provider_name;
	}
	
	/**
	 * Returns the plug-in directory path or NULL if not known.
	 * 
	 * @return the plug-in directory path or NULL
	 */
	CP_CXX_API inline const char* plugin_path() const {
		return pinfo->plugin_path;
	}
	
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
	CP_CXX_API inline const char* abi_bw_compatibility() const {
		return pinfo->abi_bw_compatibility;
	}

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
	CP_CXX_API inline const char* api_bw_compatibility() const {
		return pinfo->api_bw_compatibility;
	}
	
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
	CP_CXX_API inline const char* req_cpluff_version() const {
		return pinfo->req_cpluff_version;
	}

	/**
	 * Returns plug-in imports as a vector.
	 * 
	 * @return plug-in imports as a vector
	 */
	CP_CXX_API inline const std::vector<plugin_import>& imports() const {
		return imports_vec;
	}

    /**
     * Returns the base name of the plug-in runtime library or NULL
     * if none. A platform specific prefix (for example, "lib") and an extension
     * (for example, ".dll" or ".so") may be added to the base name.
     * The returned value corresponds to the @a library attribute of the
     * @a runtime element in a plug-in descriptor.
     * 
     * @return the base name of the plug-in runtime library or NULL
     */
	CP_CXX_API inline const char* runtime_lib_name() const {
		return pinfo->runtime_lib_name;
	}

    /**
     * Returns the name of symbol pointing to the plug-in runtime function
     * information or NULL if none. The symbol with this name should
     * point to an instance of @ref cp_plugin_runtime_t structure. This
     * corresponds to the @a funcs attribute of the @a runtime element in a
     * plug-in descriptor.
     * 
     * @return the name of the symbol pointing to the plug-in runtime information or NULL
     */
	CP_CXX_API inline const char* runtime_funcs_symbol() const {
		return pinfo->runtime_funcs_symbol;
	}

	/**
	 * Returns the extension points provided by this plug-in.
	 * 
	 * @return extension points provided by this plug-in
	 */
	CP_CXX_API inline const std::vector<ext_point_info> ext_points() const {
		return ext_points_vec;
	}
	
	/**
	 * Returns the extensions provided by this plug-in.
	 * 
	 * @return extensions provided by this plug-in
	 */
	CP_CXX_API inline const std::vector<extension_info> extensions() const {
		return extensions_vec;
	}

protected:

	CP_CXX_API ~plugin_info();

	/** @internal The C API plug-in context handle */
	cp_context_t* context;
	
	/** @internal The C API plug-in descriptor pointer */
	cp_plugin_info_t* pinfo;
	
	/** @internal The plug-in import objects */
	std::vector<plugin_import> imports_vec;
	
	/** @internal The extension point objects */
	std::vector<ext_point_info> ext_points_vec;
	
	/** @internal The extension objects */
	std::vector<extension_info> extensions_vec;

};

}

#endif /*CPLUFFXX_INFO_H_*/
