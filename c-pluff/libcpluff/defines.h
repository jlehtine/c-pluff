/*-------------------------------------------------------------------------
 * C-Pluff, a plug-in framework for C
 * Copyright 2006 Johannes Lehtinen
 *-----------------------------------------------------------------------*/

/*
 * Core internal definitions
 */

#ifndef DEFINES_H_
#define DEFINES_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#ifdef HAVE_GETTEXT
#include <libintl.h>
#endif
#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif


/* ------------------------------------------------------------------------
 * Defines
 * ----------------------------------------------------------------------*/

// Define CP_LOCAL to hide internal symbols 
#if defined(__WIN32__)
#define CP_LOCAL
#elif __GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 3)
#define CP_LOCAL __attribute__ ((visibility ("hidden")))
#else
#define CP_LOCAL
#endif

// Gettext defines 
#ifdef HAVE_GETTEXT
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
#else
#define CP_PRINTF(format_idx, arg_idx)
#define CP_CONST
#endif
#if __GNUC__ >2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 96)
#define CP_PURE __attribute__((pure))
#else
#define CP_PURE
#endif
// TODO version
#ifdef __GNUC__
#define CP_NORETURN __attribute__((noreturn))
#else
#define CP_NORETURN
#endif


#endif //DEFINES_H_
