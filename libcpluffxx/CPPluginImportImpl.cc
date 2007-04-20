#include "internalxx.h"

namespace org {
namespace cpluff {

CP_HIDDEN CPPluginImportImpl::CPPluginImportImpl(cp_plugin_import_t* pimport):
pimport(pimport) {}

CP_HIDDEN const char* CPPluginImportImpl::getPluginIdentifier() const throw () {
	return pimport->plugin_id;
}

CP_HIDDEN const char* CPPluginImportImpl::getVersion() const throw () {
	return pimport->version;
}

CP_HIDDEN bool CPPluginImportImpl::isOptional() const throw () {
	return pimport->optional;
}

}}
