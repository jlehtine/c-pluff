/*-------------------------------------------------------------------------
 * C-Pluff, a plug-in framework for C
 * Copyright 2006 Johannes Lehtinen
 *-----------------------------------------------------------------------*/

/* Generic multi-threading support */

#ifndef _THREAD_H_
#define _THREAD_H_
#ifdef CP_THREADS

#ifdef CP_THREADS_POSIX
#include <pthread.h>
#endif
#ifdef CP_THREADS_WINDOWS
#include <windows.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/


/* ------------------------------------------------------------------------
 * Data types
 * ----------------------------------------------------------------------*/

/* Building block types */
#if defined(CP_THREADS_POSIX)
typedef cpi_os_mutex_t pthread_mutex_t;
typedef cpi_os_thread_t pthread_t;
#elif defined(CP_THREADS_WINDOWS)
typedef cpi_os_mutex_t HANDLE;
typedef cpi_os_thread_t DWORD;
#else
#error Unsupported multi-threading environment
#endif

/* Mutex type */
typedef struct cpi_mutex_t {

	/** The current lock count */
	int lock_count;
	
	/** The underlying operating system mutex */
	cpi_os_mutex_t os_mutex;
	
	/** The locking thread if currently locked */
	cpi_os_thread_t os_thread;
	
} cpi_mutex_t;

/* Condition variable type */
typedef struct cpi_cond_t cpi_cond_t;


/* ------------------------------------------------------------------------
 * Function declarations
 * ----------------------------------------------------------------------*/

/* Mutex functions */

/**
 * Initializes a mutex. The mutex is initially available.
 * 
 * @param mutex the mutex
 * @return CP_OK on success, error code on failure
 */
int CP_LOCAL cpi_init_mutex(cpi_mutex_t *mutex);

/**
 * Destroys the specified mutex.
 * 
 * @param mutex the mutex
 * @return CP_OK on success, error code on failure
 */
int CP_LOCAL cpi_destroy_mutex(cpi_mutex_t *mutex);

/**
 * Waits for the specified mutex to become available and locks it.
 * 
 * @param mutex the mutex
 * @return CP_OK on success, error code on failure
 */
int CP_LOCAL cpi_lock_mutex(cpi_mutex_t *mutex);

/**
 * Unlocks the specified mutex which must have been previously locked
 * by this thread.
 * 
 * @param mutex the mutex
 * @return CP_OK on success, error code on failure
 */
int CP_LOCAL cpi_unlock_mutex(cpi_mutex_t *mutex);


/* Condition variable functions */

/**
 * Initializes a condition variable.
 * 
 * @param cond the condition variable
 * @return CP_OK on success, error code on failure
 */
int CP_LOCAL cpi_init_cond(cpi_cond_t *cond);

/**
 * Destroys the specified condition variable.
 * 
 * @param cond the condition variable
 * @return CP_OK on success, error code on failure
 */
int CP_LOCAL cpi_destroy_cond(cpi_cond_t *cond);


#ifdef __cplusplus
}
#endif /*__cplusplus */

#endif /*CP_THREADS*/
#endif /*_THREAD_H_*/
