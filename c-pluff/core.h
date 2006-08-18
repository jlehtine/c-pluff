/*-------------------------------------------------------------------------
 * C-Pluff, a plug-in framework for C
 * Copyright 2005-2006 Johannes Lehtinen
 *-----------------------------------------------------------------------*/

/*
 * Core internal declarations
 */

#ifndef _CORE_H_
#define _CORE_H_

/* Define CP_LOCAL to hide internal symbols */
#ifdef _MSC_EXTENSIONS
#define CP_LOCAL
#elif __GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 3)
#define CP_LOCAL __attribute__ ((visibility ("hidden")))
#else
#define CP_LOCAL
#endif

#include "cpluff.h"
#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

/* Gettext defines */
#ifdef HAVE_GETTEXT
#include <libintl.h>
#define _(String) dgettext(PACKAGE, String)
#define gettext_noop(String) String
#define N_(String) gettext_noop(String)
#else
#define _(String) (String)
#define N_(String) String
#define textdomain(Domain)
#define bindtextdomain(Package, Directory)
#endif /*CP_GETTEXT*/

/* Additional defines for function attributes (under GCC). */
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

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/


/* ------------------------------------------------------------------------
 * Function declarations
 * ----------------------------------------------------------------------*/


/* Locking data structures for exclusive access */

/**
 * Acquires exclusive access to C-Pluff data structures. Access is granted to
 * the calling thread. This function does not block if the calling thread
 * already has exclusive access. If access is acquired multiple times by the
 * same thread then it is only released after corresponding number of calls to
 * release.
 */
void CP_LOCAL cpi_acquire_data(void);

/**
 * Releases exclusive access to C-Pluff data structures.
 */
void CP_LOCAL cpi_release_data(void);


/* Processing errors */

/**
 * Delivers a plug-in framework error to registered error handlers.
 * 
 * @param msg the error message
 */
void CP_LOCAL cpi_error(const char *msg);

/**
 * Delivers a printf formatted plugin-in framework error to registered
 * error handlers.
 * 
 * @param msg the formatted error message
 * @param ... parameters
 */
void CP_LOCAL cpi_errorf(const char *msg, ...) CP_PRINTF(1, 2);

/**
 * Delivers a plug-in framework error to the specified error handler.
 * 
 * @param error_handler the error handler or NULL if none
 * @param msg the error message
 */
void CP_LOCAL cpi_herror(cp_error_handler_t error_handler, const char *msg);

/**
 * Delivers a printf formatted plug-in framework error to the specified
 * error handler.
 * 
 * @param error_handler the error handler or NULL if none
 * @param msg the formatted error message
 * @param ... parameters
 */
void CP_LOCAL cpi_herrorf(cp_error_handler_t error_handler, const char *msg, ...)
	CP_PRINTF(2, 3);


/* Delivering plug-in events */

/**
 * Delivers a plug-in event to registered event listeners.
 * 
 * @param event the event
 */
void CP_LOCAL cpi_deliver_event(const cp_plugin_event_t *event);


#ifdef __cplusplus
}
#endif /*__cplusplus*/

#endif /*_CORE_H_*/
