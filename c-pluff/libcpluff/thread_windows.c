/*-------------------------------------------------------------------------
 * C-Pluff, a plug-in framework for C
 * Copyright 2006 Johannes Lehtinen
 *-----------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <windows.h>
#include "cpluff.h"
#include "defines.h"
#include "util.h"
#include "internal.h"
#include "thread.h"

// Generic multi-threading support, Windows implementation 

/* ------------------------------------------------------------------------
 * Data types
 * ----------------------------------------------------------------------*/

// A generic recursive mutex implementation 
struct cpi_mutex_t {

	/// The current lock count 
	int lock_count;
	
	/// The underlying operating system mutex 
	HANDLE os_mutex;
	
	/// The condition variable for signaling availability 
	HANDLE os_cond_count;

	/// The locking thread if currently locked 
	DWORD os_thread;
	
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
	if ((mutex->os_mutex = CreateMutex(NULL, FALSE, NULL)) == NULL) {
		return NULL;
	} else if ((mutex->os_cond_count = CreateEvent(NULL, FALSE, FALSE, NULL)) == NULL) {
		int ec;
		
		ec = CloseHandle(mutex->os_mutex);
		assert(ec);
		return NULL;
	}
	return mutex;
}

CP_HIDDEN void cpi_destroy_mutex(cpi_mutex_t *mutex) {
	int ec;
	
	assert(mutex != NULL);
	assert(mutex->lock_count == 0);
	ec = CloseHandle(mutex->os_mutex);
	assert(ec);
	ec = CloseHandle(mutex->os_cond_count);
	assert(ec);
	free(mutex);
}

static char *get_win_errormsg(DWORD error, char *buffer, size_t size) {
	if (!FormatMessageA(FORMAT_MESSAGE_IGNORE_INSERTS
		| FORMAT_MESSAGE_FROM_SYSTEM,
		NULL,
		error,
		0,
		buffer,
		size / sizeof(char),
		NULL)) {
		strncpy(buffer, _("unknown error"), size);
	}
	buffer[size/sizeof(char) - 1] = '\0';
	return buffer;
}

static void lock_mutex(HANDLE mutex) {
	DWORD ec;
	
	if ((ec = WaitForSingleObject(mutex, INFINITE)) != WAIT_OBJECT_0) {
		char buffer[256];
		ec = GetLastError();
		cpi_fatalf(_("Could not lock a mutex due to error %ld: %s"),
			(long) ec, get_win_errormsg(ec, buffer, sizeof(buffer)));
	}
}

static void unlock_mutex(HANDLE mutex) {
	if (!ReleaseMutex(mutex)) {
		char buffer[256];
		DWORD ec = GetLastError();
		cpi_fatalf(_("Could not release a mutex due to error %ld: %s"),
			(long) ec, get_win_errormsg(ec, buffer, sizeof(buffer)));
	}
}

static void wait_for_event(HANDLE event) {
	if (WaitForSingleObject(event, INFINITE) != WAIT_OBJECT_0) {
		char buffer[256];
		DWORD ec = GetLastError();
		cpi_fatalf(_("Could not wait for an event due to error %ld: %s"),
			(long) ec, get_win_errormsg(ec, buffer, sizeof(buffer)));
	}
}

static void set_event(HANDLE event) {
	if (!SetEvent(event)) {
		char buffer[256];
		DWORD ec = GetLastError();
		cpi_fatalf(_("Could not set an event due to error %ld: %s"),
			(long) ec, get_win_errormsg(ec, buffer, sizeof(buffer)));
	}
}

CP_HIDDEN void cpi_lock_mutex(cpi_mutex_t *mutex) {
	DWORD self = GetCurrentThreadId();
	
	assert(mutex != NULL);
	lock_mutex(mutex->os_mutex);
	while (mutex->lock_count != 0
			&& self != mutex->os_thread) {
		unlock_mutex(mutex->os_mutex);
		wait_for_event(mutex->os_cond_count);
		lock_mutex(mutex->os_mutex);
	}
	mutex->os_thread = self;
	mutex->lock_count++;
	unlock_mutex(mutex->os_mutex);
}

CP_HIDDEN void cpi_unlock_mutex(cpi_mutex_t *mutex) {
	DWORD self = GetCurrentThreadId();
	
	assert(mutex != NULL);
	lock_mutex(mutex->os_mutex);
	if (mutex->lock_count > 0
		&& self == mutex->os_thread) {
		if (--mutex->lock_count == 0) {
			set_event(mutex->os_cond_count);
		}
	} else {
		cpi_fatalf(_("Unauthorized attempt at unlocking a mutex."));
	}
	unlock_mutex(mutex->os_mutex);
}

#if !defined(NDEBUG)
CP_HIDDEN int cpi_is_mutex_locked(cpi_mutex_t *mutex) {
	int locked;
	
	lock_mutex(mutex->os_mutex);
	locked = (mutex->lock_count != 0);
	unlock_mutex(mutex->os_mutex);
	return locked;
}
#endif
