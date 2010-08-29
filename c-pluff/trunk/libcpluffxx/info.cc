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
 * Implements container classes for static plug-in information.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cstring>
#include "internalxx.h"

namespace cpluff {

CP_HIDDEN plugin_info::plugin_info(cp_context_t* context, cp_plugin_info_t* pinfo):
context(context), pinfo(pinfo) {
	imports_vec.reserve(pinfo->num_imports);
	for (int i = 0; i < pinfo->num_imports; i++) {
		// TODO imports_vec.push_back(plugin_import(pinfo->imports + i));
	}
	ext_points_vec.reserve(pinfo->num_ext_points);
	for (int i = 0; i < pinfo->num_ext_points; i++) {
		ext_points_vec.push_back(ext_point_info(pinfo->ext_points + i));
	}
	extensions_vec.reserve(pinfo->num_extensions);
	for (int i = 0; i < pinfo->num_extensions; i++) {
		extensions_vec.push_back(extension_info(pinfo->extensions + i));
	}
}

CP_HIDDEN plugin_info::~plugin_info() {
	cp_release_info(context, pinfo);
}

CP_HIDDEN cfg_element::cfg_element(cfg_element* parent, const cp_cfg_element_t* cfge):
cfg_parent(parent), cfge(cfge) {
	for (int i = 0; i < cfge->num_atts; i += 2) {
		attr_map[cfge->atts[i]] = cfge->atts[i+1];
	}
	cfg_children.reserve(cfge->num_children);
	for (int i = 0; i < cfge->num_children; i++) {
		cfg_children.push_back(new cfg_element(this, cfge->children + i));
	}
}

}
