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

#ifndef TEST_CXX_H_
#define TEST_CXX_H_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "test.h"
#include <cpluffxx.h>


/**
 * Initializes the C-Pluff framework and creates a plug-in container.
 * Checks for any failures on the way. Also prints out context errors/warnings
 * and maintains a count of logged context errors if so requested.
 *
 * @param min_disp_sev the minimum severity of messages to be displayed
 * @param error_counter pointer to the location where the logged error count is to be stored or NULL 
 * @return the created plug-in context
 */
CP_HIDDEN shared_ptr<cpluff::plugin_container> init_container_cxx(cpluff::logger::severity min_disp_sev, int *error_counter);

#endif /*TEST_CXX_H_*/
