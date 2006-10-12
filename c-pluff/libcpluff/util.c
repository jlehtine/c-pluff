/*-------------------------------------------------------------------------
 * C-Pluff, a plug-in framework for C
 * Copyright 2005-2006 Johannes Lehtinen
 *-----------------------------------------------------------------------*/

/* Internal utility functions */

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

void CP_LOCAL cpi_process_free_ptr(list_t *list, lnode_t *node, void *dummy) {
	void *ptr = lnode_get(node);
	list_delete(list, node);
	lnode_destroy(node);
	free(ptr);
}

int CP_LOCAL cpi_version_isvalid(const char *v) {
	int i, ci;
	
	if (v == NULL) {
		return 0;
	}
	if (v[0] == '\0') {
		return 0;
	}
	i = 0;
	for (ci = 0; ci < 3; ci++) {
		if (v[i] == '0') {
			i++;
		} else if (v[i] >= '1' && v[i] <= '9') {
			while (v[i] >= '0' && v[i] <= '9') {
				i++;
			}
		} else {
			return 0;
		}
		if (v[i] == '\0') {
			return 1;
		}
		if (v[i] != '.') {
			return 0;
		}
	}
	i++;
	while (v[i] != ' ' && v[i] != '\0') {
		i++;
	}
	return v[i] == '\0';
}

int CP_LOCAL cpi_version_cmp(const char *v1, const char *v2, int nc) {
	int i1, i2, ci;
	
	i1 = 0;
	i2 = 0;
	for (ci = 0; ci < (nc < 3 ? nc : 3); ci++) {
		int si1 = i1;
		int si2 = i2;
		
		while (v1[i1] != '.' && v1[i1] != '\0') {
			i1++;
		}
		while (v2[i2] != '.' && v2[i2] != '\0') {
			i2++;
		}
		long vc1 = strtol(v1 + si1, NULL, 10);
		long vc2 = strtol(v2 + si2, NULL, 10);
		if (vc1 < vc2) {
			return -1;
		} else if (vc1 > vc2) {
			return 1;
		}
		if (v1[i1] == '\0' && v2[i2] != '\0') {
			return -1;
		} else if (v1[i1] != '\0' && v2[i2] == '\0') {
			return 1;
		} else if (v1[i1] == '\0' && v2[i2] == '\0') {
			return 0;
		}
		assert(v1[i1] == '.');
		assert(v2[i2] == '.');
		i1++;
		i2++;
	}
	if (nc < 4) {
		return 0;
	}
	return strcmp(v1 + i1, v2 + i2);
}

#ifdef HAVE_DMALLOC_H
char * CP_LOCAL cpi_strdup_dm(const char *src, const char *file, int line) {
#else
char * CP_LOCAL cpi_strdup(const char *src) {
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
