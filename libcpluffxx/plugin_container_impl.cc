#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cstdarg>
#include <cassert>
#include <cpluff.h>
#include "internalxx.h"

namespace cpluff {


CP_HIDDEN plugin_container_impl::plugin_container_impl() {
	cp_status_t status;
	context = cp_create_context(&status);
	check_cp_status(status);
	this->context = context;
}

CP_HIDDEN void plugin_container_impl::destroy() throw () {
	cp_destroy_context(context);
	delete this;
}

CP_HIDDEN void plugin_container_impl::register_plugin_collection(const char* dir) throw (api_error) {
	check_cp_status(cp_register_pcollection(context, dir));
}

CP_HIDDEN void plugin_container_impl::unregister_plugin_collection(const char* dir) throw () {
	cp_unregister_pcollection(context, dir);
}

CP_HIDDEN void plugin_container_impl::unregister_plugin_collections() throw () {
	cp_unregister_pcollections(context);
}

CP_HIDDEN plugin_info& plugin_container_impl::load_plugin_descriptor(const char* path) throw (api_error) {
	cp_status_t status;
	cp_plugin_info_t *pinfo = cp_load_plugin_descriptor(context, path, &status);
	check_cp_status(status);
	return *(new plugin_info(context, pinfo));
}

}
