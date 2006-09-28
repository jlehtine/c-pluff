/*-------------------------------------------------------------------------
 * C-Pluff, a plug-in framework for C
 * Copyright 2005-2006 Johannes Lehtinen
 *-----------------------------------------------------------------------*/

/* General purpose utility functions */

#ifndef UTIL_H_
#define UTIL_H_

#include "kazlib/list.h"
#include "kazlib/hash.h"
#include "cpluff.h"
#include "defines.h"

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/


/* ------------------------------------------------------------------------
 * Function declarations
 * ----------------------------------------------------------------------*/

/* For operating on smallish pointer sets implemented as lists */

/**
 * Compares pointers.
 * 
 * @param ptr1 the first pointer
 * @param ptr2 the second pointer
 * @return zero if the pointers are equal, otherwise non-zero
 */
int CP_LOCAL cpi_comp_ptr(const void *ptr1, const void *ptr2) CP_CONST;

/**
 * Returns a hash value for a pointer.
 * 
 * @param ptr the pointer being hashed
 * @return the corresponding hash value
 */
hash_val_t CP_LOCAL cpi_hashfunc_ptr(const void *ptr) CP_CONST;

/**
 * Adds a new pointer to a list if the pointer is not yet included.
 * 
 * @param set the set being operated on
 * @param ptr the pointer being added
 * @return non-zero if the operation was successful, zero if allocation failed
 */
int CP_LOCAL cpi_ptrset_add(list_t *set, void *ptr);

/**
 * Removes a pointer from a pointer set, if it is included.
 * 
 * @param set the set being operated on
 * @param ptr the pointer being removed
 * @return whether the pointer was contained in the set
 */
int CP_LOCAL cpi_ptrset_remove(list_t *set, const void *ptr);

/**
 * Returns whether a pointer is included in a pointer set.
 * 
 * @param set the set being operated on
 * @param ptr the pointer
 * @return non-zero if the pointer is included, zero otherwise
 */
int CP_LOCAL cpi_ptrset_contains(list_t *set, const void *ptr) CP_PURE;


/* For error handling */

/**
 * Reports a fatal error. This method does not return.
 * 
 * @param msg the formatted error message
 * @param ... parameters
 */
void CP_LOCAL cpi_fatalf(const char *msg, ...) CP_PRINTF(1, 2) CP_NORETURN;

#ifdef __cplusplus
}
#endif /*__cplusplus */

#endif /*UTIL_H_*/
