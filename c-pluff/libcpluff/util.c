/*-------------------------------------------------------------------------
 * C-Pluff, a plug-in framework for C
 * Copyright 2006 Johannes Lehtinen
 *-----------------------------------------------------------------------*/

/** @file
 * Internal utility functions
 */ 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../kazlib/list.h"
#include "cpluff.h"
#include "defines.h"
#include "util.h"


/* ------------------------------------------------------------------------
 * Function definitions
 * ----------------------------------------------------------------------*/

CP_HIDDEN int cpi_comp_ptr(const void *ptr1, const void *ptr2) {
	return !(ptr1 == ptr2);
}

CP_HIDDEN hash_val_t cpi_hashfunc_ptr(const void *ptr) {
	return (hash_val_t) ptr;
}

CP_HIDDEN int cpi_ptrset_add(list_t *set, void *ptr) {
	

	// Only add the pointer if it is not already included 
	if (cpi_ptrset_contains(set, ptr)) {
		return 1;
	} else {
		lnode_t *node;

		/* Add the pointer to the list */		
		node = lnode_create(ptr);
		if (node == NULL) {
			return 0;
		}
		list_append(set, node);
		return 1;
	}
	
}

CP_HIDDEN int cpi_ptrset_remove(list_t *set, const void *ptr) {
	lnode_t *node;
	
	// Find the pointer if it is in the set 
	node = list_find(set, ptr, cpi_comp_ptr);
	if (node != NULL) {
		list_delete(set, node);
		lnode_destroy(node);
		return 1;
	} else {
		return 0;
	}
}

CP_HIDDEN int cpi_ptrset_contains(list_t *set, const void *ptr) {
	return list_find(set, ptr, cpi_comp_ptr) != NULL;
}

CP_HIDDEN void cpi_process_free_ptr(list_t *list, lnode_t *node, void *dummy) {
	void *ptr = lnode_get(node);
	list_delete(list, node);
	lnode_destroy(node);
	free(ptr);
}

#ifdef HAVE_DMALLOC_H
CP_HIDDEN char * cpi_strdup_dm(const char *src, const char *file, int line) {
#else
CP_HIDDEN char * cpi_strdup(const char *src) {
#endif
	char *dst;
	size_t size = sizeof(char) * (strlen(src) + 1);

#ifdef HAVE_DMALLOC_H
	dst = dmalloc_malloc(file, line, size, DMALLOC_FUNC_MALLOC, 0, 0);
#else
	dst = malloc(size);
#endif
	if (dst != NULL) {
		strcpy(dst, src);
	}
	return dst;
}
