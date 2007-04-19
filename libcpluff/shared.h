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
 * Data structures and declarations shared with the C++ API
 */

#ifndef SHARED_H_
#define SHARED_H_

#include "cpluff.h"
#include "defines.h"

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus


/* ------------------------------------------------------------------------
 * Function declarations
 * ----------------------------------------------------------------------*/

/**
 * Reports a fatal error. This method does not return.
 * 
 * @param msg the formatted error message
 * @param ... parameters
 * 
 * @internal
 * This method is exported for use by the C++ interface.
 */
CP_C_API void cpi_fatalf(const char *msg, ...) CP_GCC_NORETURN CP_GCC_PRINTF(1, 2) CP_GCC_NONNULL(1);


#ifdef __cplusplus
}
#endif //__cplusplus

#endif /*SHARED_H_*/
