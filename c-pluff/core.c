/*-------------------------------------------------------------------------
 * C-Pluff, a plug-in framework for C
 * Copyright 2005-2006 Johannes Lehtinen
 *-----------------------------------------------------------------------*/

/* TODO: Rename to context.c */

/*
 * C-Pluff core functionality and variables
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdarg.h>
#include <string.h>
#ifdef CP_THREADS_POSIX
#include <pthread.h>
#endif
#ifdef CP_THREADS_WINDOWS
#include <windows.h>
#endif
#include "cpluff.h"
#include "core.h"
#include "util.h"
#include "kazlib/list.h"

/* ------------------------------------------------------------------------
 * Data types
 * ----------------------------------------------------------------------*/

/**
 * A holder structure for error handler pointers. For ISO C conformance
 * (to avoid conversions between function and object pointers) and to pass
 * context information.
 */
typedef struct eh_holder_t {
	cp_error_handler_t error_handler;
	cp_context_t *context;
} eh_holder_t;

/**
 * A holder structure for event listener pointers. For ISO C conformance
 * (to avoid conversions between function and object pointers) and to pass
 * context information.
 */
typedef struct el_holder_t {
	cp_event_listener_t event_listener;
	cp_context_t *context;
} el_holder_t;

/**
 * A holder structure for error handler parameters.
 */
typedef struct eh_params_t {
	cp_context_t *context;
	char *msg;
} eh_params_t;


/* ------------------------------------------------------------------------
 * Static function declarations
 * ----------------------------------------------------------------------*/

/**
 * Processes a node by freeing the associated eh_holder_t and deleting
 * the node from the list.
 * 
 * @param list the list being processed
 * @param node the node being processed
 * @param dummy not used
 */
static void process_free_eh_holder(list_t *list, lnode_t *node, void *dummy);

/**
 * Processes a node by freeing the associated el_holder_t and deleting the
 * node from the list.
 * 
 * @param list the list being processed
 * @param node the node being processed
 * @param dummy not used
 */
static void process_free_el_holder(list_t *list, lnode_t *node, void *dummy);

/**
 * Compares error handler holders.
 * 
 * @param h1 the first holder to be compared
 * @param h2 the second holder to be compared
 * @return zero if the holders point to the same function, otherwise non-zero
 */
static int comp_eh_holder(const void *h1, const void *h2) CP_PURE;

/**
 * Compares event listener holders.
 * 
 * @param h1 the first holder to be compared
 * @param h2 the second holder to be compared
 * @return zero if the holders point to the same function, otherwise non-zero
 */
static int comp_el_holder(const void *h1, const void *h2) CP_PURE;

/**
 * Processes a node by delivering the specified error message to the associated
 * error handler.
 * 
 * @param list the list being processed
 * @param node the node being processed
 * @param msg the error message
 */
static void process_error(list_t *list, lnode_t *node, void *msg);

/**
 * Processes a node by delivering the specified event to the associated
 * event listener.
 * 
 * @param list the list being processed
 * @param node the node being processed
 * @param event the event
 */
static void process_event(list_t *list, lnode_t *node, void *event);


#ifdef CP_THREADS

/**
 * Locks the data mutex.
 */
static void lock_mutex(void);

/**
 * Unlocks the data mutex.
 */
static void unlock_mutex(void);

#endif

#ifdef CP_THREADS_WINDOWS

/**
 * Returns the system error message corresponding to the specified Windows
 * error code.
 * 
 * @param error the system error code
 * @param buffer buffer for the error message
 * @param size the size of buffer in bytes
 * @return the filled buffer
 */
static char *get_win_errormsg(DWORD error, char *buffer, size_t size);

#endif

/* ------------------------------------------------------------------------
 * Variables
 * ----------------------------------------------------------------------*/

/** Whether gettext has been initialized */
static int gettext_initialized = 0;

/* Recursive mutex implementation for data access */
#if defined(CP_THREADS_POSIX)
static pthread_mutex_t data_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t data_cond = PTHREAD_COND_INITIALIZER;
static unsigned int data_lock_count = 0;
static pthread_t data_thread;
#elif defined(CP_THREADS_WINDOWS)
static HANDLE data_mutex = NULL;
static HANDLE data_event = NULL;
static unsigned int data_wait_count = 0;
static unsigned int data_lock_count = 0;
static DWORD data_thread;
#endif


/* ------------------------------------------------------------------------
 * Function definitions
 * ----------------------------------------------------------------------*/


/* Generic */

static void process_free_eh_holder(list_t *list, lnode_t *node, void *dummy) {
	eh_holder_t *h = lnode_get(node);
	list_delete(list, node);
	lnode_destroy(node);
	free(h);
}

static void process_free_el_holder(list_t *list, lnode_t *node, void *dummy) {
	el_holder_t *h = lnode_get(node);
	list_delete(list, node);
	lnode_destroy(node);
	free(h);
}

static int comp_eh_holder(const void *h1, const void *h2) {
	const eh_holder_t *ehh1 = h1;
	const eh_holder_t *ehh2 = h2;
	
	return (ehh1->error_handler != ehh2->error_handler);
}

static int comp_el_holder(const void *h1, const void *h2) {
	const el_holder_t *elh1 = h1;
	const el_holder_t *elh2 = h2;
	
	return (elh1->event_listener != elh2->event_listener);
}

static void process_error(list_t *list, lnode_t *node, void *msg) {
	eh_holder_t *h = lnode_get(node);
	h->error_handler(h->context, msg);
}

static void process_event(list_t *list, lnode_t *node, void *event) {
	el_holder_t *h = lnode_get(node);
	h->event_listener(h->context, event);
}


/* Initialization and destroy */

cp_context_t * CP_API cp_create_context(cp_error_handler_t error_handler, int *error) {
	cp_context_t *context = NULL;
	int status = CP_OK;

	/* Initialize internal state */
	do {
	
		/* Gettext initialization */
		// TODO: threading
		if (!gettext_initialized) {
			gettext_initialized = 1;
			bindtextdomain(PACKAGE, CP_DATADIR "/locale");
		}
		
		/* Allocate memory for the context */
		if ((context = malloc(sizeof(cp_context_t))) == NULL) {
			status = CP_ERR_RESOURCE;
			break;
		}		
	
		/* Initialize data structures as necessary */
		memset(context, 0, sizeof(cp_context_t));
		context->error_handlers = list_create(LISTCOUNT_T_MAX);
		context->event_listeners = list_create(LISTCOUNT_T_MAX);
		context->plugins = hash_create(HASHCOUNT_T_MAX,
			(int (*)(const void *, const void *)) strcmp, NULL);
		context->started_plugins = list_create(LISTCOUNT_T_MAX);
		context->used_plugins = hash_create(HASHCOUNT_T_MAX,
			cpi_comp_ptr, cpi_hashfunc_ptr);
		if (context->error_handlers == NULL
			|| context->event_listeners == NULL
			|| context->plugins == NULL
			|| context->started_plugins == NULL
			|| context->used_plugins == NULL) {
			status = CP_ERR_RESOURCE;
			break;
		}

#ifdef CP_THREADS_WINDOWS
		/* Initialize IPC resources */
		if ((data_mutex = CreateMutex(NULL, FALSE, NULL)) != NULL) {
			data_event = CreateEvent(NULL, TRUE, FALSE, NULL);
		}
		if (data_event == NULL) {
			if (error_handler != NULL) {
				char buffer[256];
				DWORD ec = GetLastError();				
				cpi_herrorf(NULL, error_handler, _("IPC resources could not be initialized due to error %ld: %s"), (long) ec, get_win_errormsg(ec, buffer, sizeof(buffer));
			}
			status = CP_ERR_RESOURCE;
			break;
		}
#endif
	
		/* Register initial error handler */
		if (error_handler != NULL) {
			if (cp_add_error_handler(context, error_handler) != CP_OK) {
				status = CP_ERR_RESOURCE;
				break;
			}
		}
		
	} while (0);
	
	/* Report failure */
	switch (status) {
		
		case CP_ERR_RESOURCE:
			cpi_herror(NULL, error_handler, _("Plug-in context could not be created due to insufficient system resources."));
			break;
	}
	
	/* Rollback initialization on failure */
	if (status != CP_OK && context != NULL) {
		cp_destroy_context(context);
		context = NULL;
	}

	/* Return the final status */
	if (error != NULL) {
		*error = status;
	}
	
	/* Return the context (or NULL on failure) */
	return context;
}

void CP_API cp_destroy_context(cp_context_t *context) {

	/* Unload all plug-ins */
	if (context->plugins != NULL && !hash_isempty(context->plugins)) {
		cp_unload_all_plugins(context);
	}
	
	/* Release data structures */
	if (context->plugins != NULL) {
		assert(hash_isempty(context->plugins));
		hash_destroy(context->plugins);
		context->plugins = NULL;
	}
	if (context->started_plugins != NULL) {
		assert(list_isempty(context->started_plugins));
		list_destroy(context->started_plugins);
		context->started_plugins = NULL;
	}
	if (context->used_plugins != NULL) {
		hash_free_nodes(context->used_plugins);
		hash_destroy(context->used_plugins);
		context->used_plugins = NULL;
	}
	if (context->error_handlers != NULL) {
		list_process(context->error_handlers, NULL, process_free_eh_holder);
		list_destroy(context->error_handlers);
		context->error_handlers = NULL;
	}
	if (context->event_listeners != NULL) {
		list_process(context->event_listeners, NULL, process_free_el_holder);
		list_destroy(context->event_listeners);
		context->event_listeners = NULL;
	}

#ifdef CP_THREADS_WINDOWS
	/* Release IPC resources */
	if (data_event != NULL) {
		CloseHandle(data_event);
		data_event = NULL;
	}
	if (data_mutex != NULL) {
		CloseHandle(data_mutex);
		data_mutex = NULL;
	}
#endif
	
}


/* Error handling */

int CP_API cp_add_error_handler(cp_context_t *context, cp_error_handler_t error_handler) {
	int status = CP_ERR_RESOURCE;
	eh_holder_t *holder;
	lnode_t *node;
	
	assert(error_handler != NULL);
	
	cpi_lock_context(context);
	if ((holder = malloc(sizeof(eh_holder_t))) != NULL) {
		holder->error_handler = error_handler;
		if ((node = lnode_create(holder)) != NULL) {
			list_append(context->error_handlers, node);
			status = CP_OK;
		} else {
			free(holder);
		}
	}
	cpi_unlock_context(context);
	if (status != CP_OK) {
		cpi_error(context,_("An error handler could not be registered due to insufficient system resources."));
	}
	return status;
}

int CP_API cp_remove_error_handler(cp_context_t *context, cp_error_handler_t error_handler) {
	eh_holder_t holder;
	lnode_t *node;
	
	holder.error_handler = error_handler;
	cpi_lock_context(context);
	node = list_find(context->error_handlers, &holder, comp_eh_holder);
	if (node != NULL) {
		process_free_eh_holder(context->error_handlers, node, NULL);
	}
	cpi_unlock_context(context);
	return CP_OK;
}

void CP_LOCAL cpi_error(cp_context_t *context, const char *msg) {
	assert(msg != NULL);
	cpi_lock_context(context);
	list_process(context->error_handlers, (void *) msg, process_error);
	cpi_unlock_context(context);
}

void CP_LOCAL cpi_errorf(cp_context_t *context, const char *msg, ...) {
	va_list params;
	char fmsg[256];
	
	assert(msg != NULL);
	va_start(params, msg);
	vsnprintf(fmsg, sizeof(fmsg), msg, params);
	va_end(params);
	fmsg[sizeof(fmsg)/sizeof(char) - 1] = '\0';
	cpi_error(context, fmsg);
}

void CP_LOCAL cpi_herror(cp_context_t *context, cp_error_handler_t error_handler, const char *msg) {
	assert(msg != NULL);
	if (error_handler != NULL) {
		error_handler(context, msg);
	}
}

void CP_LOCAL cpi_herrorf(cp_context_t *context, cp_error_handler_t error_handler, const char *msg, ...) {
	va_list params;
	char fmsg[256];
	
	assert(msg != NULL);
	va_start(params, msg);
	vsnprintf(fmsg, sizeof(fmsg), msg, params);
	va_end(params);
	fmsg[sizeof(fmsg)/sizeof(char) - 1] = '\0';
	cpi_herror(context, error_handler, msg);
}


/* Event listeners */

int CP_API cp_add_event_listener(cp_context_t *context, cp_event_listener_t event_listener) {
	int status = CP_ERR_RESOURCE;
	el_holder_t *holder;
	lnode_t *node;

	assert(event_listener != NULL);
	
	cpi_lock_context(context);
	if ((holder = malloc(sizeof(el_holder_t))) != NULL) {
		holder->event_listener = event_listener;
		if ((node = lnode_create(holder)) != NULL) {
			list_append(context->event_listeners, node);
			status = CP_OK;
		} else {
			free(holder);
		}
	}
	cpi_unlock_context(context);
	if (status != CP_OK) {
		cpi_error(context, _("An event listener could not be registered due to insufficient system resources."));
	}
	return status;
}

int CP_API cp_remove_event_listener(cp_context_t *context, cp_event_listener_t event_listener) {
	el_holder_t holder;
	lnode_t *node;
	
	holder.event_listener = event_listener;
	cpi_lock_context(context);
	node = list_find(context->event_listeners, &holder, comp_el_holder);
	if (node != NULL) {
		process_free_el_holder(context->event_listeners, node, NULL);
	}
	cpi_unlock_context(context);
	return CP_OK;
}

void CP_LOCAL cpi_deliver_event(cp_context_t *context, const cp_plugin_event_t *event) {
	assert(event != NULL);
	assert(event->plugin_id != NULL);
	cpi_lock_context(context);
	list_process(context->event_listeners, (void *) event, process_event);
	cpi_unlock_context(context);
}


/* Locking */

void CP_LOCAL cpi_lock_context(cp_context_t *context) {
#if defined(CP_THREADS_POSIX)
	pthread_t self = pthread_self();
	lock_mutex();
	while (data_lock_count > 0 && !pthread_equal(self, data_thread)) {
		pthread_cond_wait(&data_cond, &data_mutex);
	}
	data_thread = self;
	data_lock_count++;
	unlock_mutex();
#elif defined(CP_THREADS_WINDOWS)
	DWORD self = GetCurrentThreadId();
	lock_mutex();
	while (data_lock_count > 0 && data_thread != self) {
		DWORD ec;
		data_wait_count++;
		unlock_mutex();
		if ((ec = WaitForSingleObject(data_event, INFINITE)) != WAIT_OBJECT_0) {
			char buffer[256];
			ec = GetLastError();
			fprintf(stderr,
				_(PACKAGE_NAME ": FATAL: Could not wait for an event due to error %ld: %s\n"),
				(long) ec, get_win_errormsg(ec, buffer, sizeof(buffer)));
			exit(1);
		}
		lock_mutex();
		data_wait_count--;
		if (data_wait_count == 0) {
			if (!ResetEvent(data_event)) {
				char buffer[256];
				ec = GetLastError();
				fprintf(stderr, _(PACKAGE_NAME ": FATAL: Could not reset an event due to error %ld: %s\n"),
					(long) ec, get_win_errormsg(ec, buffer, sizeof(buffer)));
				exit(1);
			}
		}
	}
	data_thread = self;
	data_lock_count++;
	unlock_mutex();
#endif
}

void CP_LOCAL cpi_unlock_context(cp_context_t *context) {
#if defined(CP_THREADS_POSIX)
	lock_mutex();
	assert(pthread_equal(pthread_self(), data_thread));
	assert(data_lock_count > 0);
	data_lock_count--;
	if (data_lock_count == 0) {
		pthread_cond_broadcast(&data_cond);
	}
	unlock_mutex();
#elif defined(CP_THREADS_WINDOWS)
	lock_mutex();
	assert(data_thread == GetCurrentThreadId());
	assert(data_lock_count > 0);
	data_lock_count--;
	if (data_lock_count == 0 && data_wait_count > 0) {
		if (!SetEvent(data_event)) {
			char buffer[256];
			DWORD ec = GetLastError();
			fprintf(stderr, _(PACKAGE_NAME " FATAL: Could not signal an event due to error %ld: %s\n"),
				(long) ec, get_win_errormsg(ec, buffer, sizeof(buffer)));
			exit(1);
		}
	}
	unlock_mutex();
#endif
}

#if defined(CP_THREADS_POSIX) || defined(CP_THREADS_WINDOWS)

static void lock_mutex(void) {
#if defined(CP_THREADS_POSIX)
	int ec;
	if ((ec = pthread_mutex_lock(&data_mutex))) {
		fprintf(stderr,
			_(PACKAGE_NAME ": FATAL: Could not lock a mutex due to error %d.\n"), ec);
		exit(1);
	}
#elif defined(CP_THREADS_WINDOWS)
	DWORD ec;
	if ((ec = WaitForSingleObject(data_mutex, INFINITE)) != WAIT_OBJECT_0) {
		char buffer[256];
		ec = GetLastError();
		fprintf(stderr,
			_(PACKAGE_NAME ": FATAL: Could not lock a mutex due to error %ld: %s\n"),
			(long) ec, get_win_errormsg(ec, buffer, sizeof(buffer)));
		exit(1);
	}
#endif
}

static void unlock_mutex(void) {
#if defined(CP_THREADS_POSIX)
	int ec;
	if ((ec = pthread_mutex_unlock(&data_mutex))) {
		fprintf(stderr,
			_(PACKAGE_NAME ": FATAL: Could not unlock a mutex due to error %d.\n"), ec);
		exit(1);
	}
#elif defined(CP_THREADS_WINDOWS)
	if (!ReleaseMutex(data_mutex)) {
		char buffer[256];
		DWORD ec = GetLastError();
		fprintf(stderr, _(PACKAGE_NAME ": FATAL: Could not release a mutex due to error %ld: %s\n"),
			(long) ec, get_win_errormsg(ec, buffer, sizeof(buffer)));
		exit(1);
	}
#endif
}

#ifdef CP_THREADS_WINDOWS
static void get_win_errormsg(DWORD error, char *buffer, size_t size) {
	if (!FormatMessageA(FORMAT_MESSAGE_IGNORE_INSERTS
		| FORMAT_MESSAGE_SYSTEM,
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
#endif /*CP_THREADS_WINDOWS*/

#endif
