#include "internalxx.h"

namespace org {
namespace cpluff {

CP_HIDDEN CPPluginDescriptorImpl::CPPluginDescriptorImpl(CPPluginContextImpl& context, cp_plugin_info_t *pinfo):
CPReferenceCountedImpl(context), pinfo(pinfo) {
	imports.reserve(pinfo->num_imports);
	int i;
	for (i = 0; i < pinfo->num_imports; i++) {
		imports.push_back(new CPPluginImportImpl(pinfo->imports + i));
	}
	/*
	extensionPoints.reserve(pinfo->num_ext_points);
	for (i = 0; i < pinfo->num_ext_points; i++) {
		extensionPoints.push_back(new CPExtensionPointDescriptorImpl(pinfo->ext_points + i));
	}
	*/
}

CP_HIDDEN const char* CPPluginDescriptorImpl::getIdentifier() const throw () {
	return pinfo->identifier;
}

CP_HIDDEN const char* CPPluginDescriptorImpl::getName() const throw () {
	return pinfo->name;
}

CP_HIDDEN const char* CPPluginDescriptorImpl::getVersion() const throw() {
	return pinfo->version;
}

CP_HIDDEN const char* CPPluginDescriptorImpl::getProviderName() const throw () {
	return pinfo->provider_name;
}
	
CP_HIDDEN const char* CPPluginDescriptorImpl::getPath() const throw () {
	return pinfo->plugin_path;
}
	
CP_HIDDEN const char* CPPluginDescriptorImpl::getABIBackwardsCompatibility() const throw () {
	return pinfo->abi_bw_compatibility;
}

CP_HIDDEN const char* CPPluginDescriptorImpl::getAPIBackwardsCompatibility() const throw () {
	return pinfo->api_bw_compatibility;
}
	
CP_HIDDEN const char* CPPluginDescriptorImpl::getRequiredCPluffVersion() const throw () {
	return pinfo->req_cpluff_version;
}

CP_HIDDEN const std::vector<const CPPluginImport*>& CPPluginDescriptorImpl::getImports() const throw () {
	return imports;
}

CP_HIDDEN const char* CPPluginDescriptorImpl::getRuntimeLibraryName() const throw () {
	return pinfo->runtime_lib_name;
}

CP_HIDDEN const char* CPPluginDescriptorImpl::getRuntimeFunctionsSymbol() const throw () {
	return pinfo->runtime_funcs_symbol;
}

CP_HIDDEN const std::vector<const CPExtensionPointDescriptor*>& CPPluginDescriptorImpl::getExtensionPoints() const throw () {
	return extensionPoints;
}
	
CP_HIDDEN const std::vector<const CPExtensionDescriptor*>& CPPluginDescriptorImpl::getExtensions() const throw () {
	return extensions;
}

CP_HIDDEN CPPluginDescriptorImpl::~CPPluginDescriptorImpl() throw () {
	std::vector<const CPPluginImport*>::iterator iter;
	for (iter = imports.begin(); iter != imports.end(); iter++) {
		delete *iter;
	}
	cp_release_info(context.getCContext(), pinfo);
}

}}
