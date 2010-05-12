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
 * Preprocessor definitions for C-Pluff C++ API.
 */

#ifndef CPLUFFXX_DEFINES_H_
#define CPLUFFXX_DEFINES_H_

/**
 * @defgroup cxxDefines Defines
 * Preprocessor defines.
 */

#include <cpluffdef.h>

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

#endif /*CPLUFFXX_DEFINES_H_*/
