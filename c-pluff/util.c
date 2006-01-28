/*-------------------------------------------------------------------------
 * C-Pluff, a plug-in framework for C
 * Copyright 2005-2006 Johannes Lehtinen
 *-----------------------------------------------------------------------*/

#include <stdio.h>
#include "util.h"
#include "kazlib/list.h"


/* ------------------------------------------------------------------------
 * Static function declarations
 * ----------------------------------------------------------------------*/

/**
 * Compares pointers.
 * 
 * @param ptr1 the first pointer
 * @param ptr2 the second pointer
 * @return zero if the pointers are equal, otherwise non-zero
 */
static int comp_ptr(const void *ptr1, const void *ptr2) CP_CONST;

/* ------------------------------------------------------------------------
 * Function definitions
 * ----------------------------------------------------------------------*/

static int comp_ptr(const void *ptr1, const void *ptr2) {
	return !(ptr1 == ptr2);
}

int cpi_ptrset_add(list_t *set, void *ptr) {
	

	/* Only add the pointer if it is not already included */
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

int cpi_ptrset_remove(list_t *set, const void *ptr) {
	lnode_t *node;
	
	/* Find the pointer if it is in the set */
	node = list_find(set, ptr, comp_ptr);
	if (node != NULL) {
		list_delete(set, node);
		lnode_destroy(node);
		return 1;
	} else {
		return 0;
	}
}

int cpi_ptrset_contains(list_t *set, const void *ptr) {
	return list_find(set, ptr, comp_ptr) != NULL;
}
