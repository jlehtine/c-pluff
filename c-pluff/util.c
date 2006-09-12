/*-------------------------------------------------------------------------
 * C-Pluff, a plug-in framework for C
 * Copyright 2005-2006 Johannes Lehtinen
 *-----------------------------------------------------------------------*/

#include <stdio.h>
#include "defines.h"
#include "util.h"
#include "kazlib/list.h"


/* ------------------------------------------------------------------------
 * Static function declarations
 * ----------------------------------------------------------------------*/

/* ------------------------------------------------------------------------
 * Function definitions
 * ----------------------------------------------------------------------*/

int CP_LOCAL cpi_comp_ptr(const void *ptr1, const void *ptr2) {
	return !(ptr1 == ptr2);
}

hash_val_t CP_LOCAL cpi_hashfunc_ptr(const void *ptr) {
	return (hash_val_t) ptr;
}

int CP_LOCAL cpi_ptrset_add(list_t *set, void *ptr) {
	

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

int CP_LOCAL cpi_ptrset_remove(list_t *set, const void *ptr) {
	lnode_t *node;
	
	/* Find the pointer if it is in the set */
	node = list_find(set, ptr, cpi_comp_ptr);
	if (node != NULL) {
		list_delete(set, node);
		lnode_destroy(node);
		return 1;
	} else {
		return 0;
	}
}

int CP_LOCAL cpi_ptrset_contains(list_t *set, const void *ptr) {
	return list_find(set, ptr, cpi_comp_ptr) != NULL;
}
