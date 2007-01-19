/*-------------------------------------------------------------------------
 * C-Pluff, a plug-in framework for C
 * Copyright 2006 Johannes Lehtinen
 *-----------------------------------------------------------------------*/

/** @file
 * Core internal defines
 */

#ifndef DEFINES_H_
#define DEFINES_H_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#ifdef ENABLE_NLS
#include <libintl.h>
#endif
#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif


/* ------------------------------------------------------------------------
 * Defines
 * ----------------------------------------------------------------------*/

// Gettext defines 
#ifdef ENABLE_NLS
#define _(String) dgettext(PACKAGE, String)
#define gettext_noop(String) String
#define N_(String) gettext_noop(String)
#else
#define _(String) (String)
#define N_(String) String
#define textdomain(Domain)
#define bindtextdomain(Package, Directory)
#endif //HAVE_GETTEXT


// Additional defines for function attributes (under GCC). 
#if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 5)
#define CP_PRINTF(format_idx, arg_idx) \
	__attribute__((format (printf, format_idx, arg_idx)))
#define CP_CONST __attribute__((const))
#define CP_NORETURN __attribute__((noreturn))
#else
#define CP_PRINTF(format_idx, arg_idx)
#define CP_CONST
#define CP_NORETURN
#endif
#if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 96)
#define CP_PURE __attribute__((pure))
#else
#define CP_PURE
#endif


#endif //DEFINES_H_
