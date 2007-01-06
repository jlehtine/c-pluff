/*-------------------------------------------------------------------------
 * C-Pluff, a plug-in framework for C
 * Copyright 2006 Johannes Lehtinen
 *-----------------------------------------------------------------------*/

/** @file
 * Posix implementation for generic mutex functions
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>
#include "cpluff.h"
#include "defines.h"
#include "util.h"
#include "internal.h"
#include "thread.h"


/* ------------------------------------------------------------------------
 * Data types
 * ----------------------------------------------------------------------*/

// A generic recursive mutex implementation 
struct cpi_mutex_t {

	/// The current lock count 
	int lock_count;
	
	/// The underlying operating system mutex 
	pthread_mutex_t os_mutex;
	
	/// The condition variable for signaling availability 
	pthread_cond_t os_cond_count;

	/// The locking thread if currently locked 
	pthread_t os_thread;
	
};


/* ------------------------------------------------------------------------
 * Function definitions
 * ----------------------------------------------------------------------*/

CP_HIDDEN cpi_mutex_t * cpi_create_mutex(void) {
	cpi_mutex_t *mutex;
	
	if ((mutex = malloc(sizeof(cpi_mutex_t))) == NULL) {
		return NULL;
	}
	memset(mutex, 0, sizeof(cpi_mutex_t));
	if (pthread_mutex_init(&(mutex->os_mutex), NULL)) {
		return NULL;
	} else if (pthread_cond_init(&(mutex->os_cond_count), NULL)) {
		int ec;
		
		ec = pthread_mutex_destroy(&(mutex->os_mutex));
		assert(!ec);
		return NULL;
	}
	return mutex;
}

CP_HIDDEN void cpi_destroy_mutex(cpi_mutex_t *mutex) {
	int ec;
	
	assert(mutex != NULL);
	assert(mutex->lock_count == 0);
	ec = pthread_mutex_destroy(&(mutex->os_mutex));
	assert(!ec);
	ec = pthread_cond_destroy(&(mutex->os_cond_count));
	assert(!ec);
	free(mutex);
}

static void lock_mutex(pthread_mutex_t *mutex) {
	int ec;
	
	if ((ec = pthread_mutex_lock(mutex))) {
		cpi_fatalf(_("Could not lock a mutex due to error %d."), ec);
	}
}

static void unlock_mutex(pthread_mutex_t *mutex) {
	int ec;
	
	if ((ec = pthread_mutex_unlock(mutex))) {
		cpi_fatalf(_("Could not unlock a mutex due to error %d."), ec);
	}
}

CP_HIDDEN void cpi_lock_mutex(cpi_mutex_t *mutex) {
	pthread_t self = pthread_self();
	
	assert(mutex != NULL);
	lock_mutex(&(mutex->os_mutex));
	while (mutex->lock_count != 0
			&& !pthread_equal(self, mutex->os_thread)) {
		int ec;
		
		if ((ec = pthread_cond_wait(&(mutex->os_cond_count), &(mutex->os_mutex)))) {
			cpi_fatalf(_("Could not wait for a condition variable due to error %d."), ec);
		}
	}
	mutex->os_thread = self;
	mutex->lock_count++;
	unlock_mutex(&(mutex->os_mutex));
}

CP_HIDDEN void cpi_unlock_mutex(cpi_mutex_t *mutex) {
	pthread_t self = pthread_self();
	
	assert(mutex != NULL);
	lock_mutex(&(mutex->os_mutex));
	if (mutex->lock_count > 0
		&& pthread_equal(self, mutex->os_thread)) {
		if (--mutex->lock_count == 0) {
			int ec;
			
			if ((ec = pthread_cond_signal(&(mutex->os_cond_count)))) {
				cpi_fatalf(_("Could not signal a condition variable due to error %d."), ec);
			}
		}
	} else {
		cpi_fatalf(_("Unauthorized attempt at unlocking a mutex."));
	}
	unlock_mutex(&(mutex->os_mutex));
}

#if !defined(NDEBUG)
CP_HIDDEN int cpi_is_mutex_locked(cpi_mutex_t *mutex) {
	int locked;
	
	lock_mutex(&(mutex->os_mutex));
	locked = (mutex->lock_count != 0);
	unlock_mutex(&(mutex->os_mutex));
	return locked;
}
#endif
