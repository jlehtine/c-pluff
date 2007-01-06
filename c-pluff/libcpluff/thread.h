/*-------------------------------------------------------------------------
 * C-Pluff, a plug-in framework for C
 * Copyright 2006 Johannes Lehtinen
 *-----------------------------------------------------------------------*/

/** @file
 * Declarations for generic mutex functions and types
 */ 

#ifndef THREAD_H_
#define THREAD_H_
#ifdef CP_THREADS

#include "defines.h"

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus


/* ------------------------------------------------------------------------
 * Data types
 * ----------------------------------------------------------------------*/

// A generic mutex implementation 
typedef struct cpi_mutex_t cpi_mutex_t;


/* ------------------------------------------------------------------------
 * Function declarations
 * ----------------------------------------------------------------------*/

// Mutex functions 

/**
 * Creates a mutex. The mutex is initially available.
 * 
 * @return the created mutex or NULL if no resources available
 */
CP_HIDDEN cpi_mutex_t * cpi_create_mutex(void);

/**
 * Destroys the specified mutex.
 * 
 * @param mutex the mutex
 */
CP_HIDDEN void cpi_destroy_mutex(cpi_mutex_t *mutex);

/**
 * Waits for the specified mutex to become available and locks it.
 * If the calling thread has already locked the mutex then the
 * lock count of the mutex is increased.
 * 
 * @param mutex the mutex
 */
CP_HIDDEN void cpi_lock_mutex(cpi_mutex_t *mutex);

/**
 * Unlocks the specified mutex which must have been previously locked
 * by this thread. If there has been several calls to cpi_lock_mutex
 * by the same thread then the mutex is unlocked only after corresponding
 * number of unlock requests.
 * 
 * @param mutex the mutex
 */
CP_HIDDEN void cpi_unlock_mutex(cpi_mutex_t *mutex);

#if !defined(NDEBUG)

/**
 * Returns whether the mutex is currently locked. This function
 * is only intended to be used for assertions. The returned state
 * reflects the state of the mutex only at the time of inspection.
 */
CP_HIDDEN int cpi_is_mutex_locked(cpi_mutex_t *mutex);

#endif

#ifdef __cplusplus
}
#endif //__cplusplus 

#endif //CP_THREADS
#endif //THREAD_H_
