/*-------------------------------------------------------------------------
 * C-Pluff, a plug-in framework for C
 * Copyright 2006 Johannes Lehtinen
 *-----------------------------------------------------------------------*/

/*
 * Dynamic plug-in symbols
 */

#include <string.h>
#include <assert.h>
#include "../kazlib/hash.h"
#include "cpluff.h"
#include "defines.h"
#include "internal.h"
#include "util.h"


/* ------------------------------------------------------------------------
 * Data structures
 * ----------------------------------------------------------------------*/

/// Information about symbol providing plug-in
typedef struct symbol_provider_info_t {
	
	// Whether there is also an import dependency for the plug-in
	int imported;
	
	// Total symbol usage count
	int usage_count;
	
	// Maps used symbols into symbol specific usage count information
	hash_t *symbols;
	
} symbol_provider_info_t;

/// Information about used symbol
typedef struct symbol_info_t {
	
	// Symbol usage count
	int usage_count;
	
	// The name of the symbol
	char *name;
	
} symbol_info_t;


/* ------------------------------------------------------------------------
 * Function definitions
 * ----------------------------------------------------------------------*/

void * CP_API cp_resolve_symbol(cp_context_t *context, const char *id, const char *name, int *error) {
	int status = CP_OK;
	int error_reported = 1;
	hnode_t *node;
	void *symbol = NULL;
	symbol_info_t *symbol_info = NULL;
	symbol_provider_info_t *provider_info = NULL;
	cp_plugin_t *pp = NULL;

	assert(context != NULL);	
	assert(id != NULL);
	assert(name != NULL);
	
	// Resolve the symbol
	cpi_check_invocation(context, __func__);
	cpi_lock_context(context);
	do {

		// Look up the symbol defining plug-in
		node = hash_lookup(context->env->plugins, id);
		if (node == NULL) {
			cpi_warnf(context, _("Symbol %s in unknown plug-in %s could not be resolved."), name, id);
			status = CP_ERR_UNKNOWN;
			break;
		}
		pp = hnode_get(node);

		// Make sure the plug-in has been started
		if ((status = cpi_start_plugin(context, pp)) != CP_OK) {
			cpi_errorf(context, _("Symbol %s in plug-in %s could not be resolved because the plug-in could not be started."), name, id);
			error_reported = 1;
			break;
		}

		// Look up the symbol
		if (pp->runtime_lib == NULL) {
			cpi_warnf(context, _("Symbol %s in plug-in %s could not be resolved because the plug-in does not have runtime library."), name, id);
			status = CP_ERR_UNKNOWN;
			break;
		}
		if ((symbol = DLSYM(pp->runtime_lib, name)) == NULL) {
			cpi_warnf(context, _("Symbol %s in plug-in %s could not be resolved because it is not defined."), name, id);
			status = CP_ERR_UNKNOWN;
			break;
		}

		// Lookup or initialize symbol provider information
		if ((node = hash_lookup(context->symbol_providers, pp)) != NULL) {
			provider_info = hnode_get(node);
		} else {
			if ((provider_info = malloc(sizeof(symbol_provider_info_t))) == NULL) {
				status = CP_ERR_RESOURCE;
				break;
			}
			memset(provider_info, 0, sizeof(symbol_provider_info_t));
			provider_info->imported = cpi_ptrset_contains(context->plugin->imported, pp);
			provider_info->symbols = hash_create(HASHCOUNT_T_MAX, cpi_comp_ptr, cpi_hashfunc_ptr);
			if (provider_info->symbols == NULL) {
				status = CP_ERR_RESOURCE;
				break;
			}
			if (!hash_alloc_insert(context->symbol_providers, pp, provider_info)) {
				status = CP_ERR_RESOURCE;
				break;
			}
		}
		
		// Lookup or initialize symbol information
		if ((node = hash_lookup(provider_info->symbols, symbol)) != NULL) {
			symbol_info = hnode_get(node);
		} else {
			if ((symbol_info = malloc(sizeof(symbol_info_t))) == NULL) {
				status = CP_ERR_RESOURCE;
				break;
			}
			memset(symbol_info, 0, sizeof(symbol_info_t));
			symbol_info->usage_count = 0;
			symbol_info->name = cpi_strdup(name);
			if (symbol_info->name == NULL) {
				status = CP_ERR_RESOURCE;
				break;
			}
			if (!hash_alloc_insert(provider_info->symbols, symbol, symbol_info)) {
				status = CP_ERR_RESOURCE;
				break;
			}
		}
		
		// Add dependencies
		if (!provider_info->imported && provider_info->usage_count == 0) {
			if (!cpi_ptrset_add(context->plugin->imported, pp)) {
				status = CP_ERR_RESOURCE;
				break;
			}
			if (!cpi_ptrset_add(pp->importing, context->plugin)) {
				cpi_ptrset_remove(context->plugin->imported, pp);
				status = CP_ERR_RESOURCE;
				break;
			}
		}
		
		// Increase usage counts
		symbol_info->usage_count++;
		provider_info->usage_count++;

	} while (0);
	cpi_unlock_context(context);

	// Clean up
	if (symbol_info != NULL && symbol_info->usage_count == 0) {
		if ((node = hash_lookup(provider_info->symbols, symbol)) != NULL) {
			hash_delete_free(provider_info->symbols, node);
		}
		if (symbol_info->name != NULL) {
			free(symbol_info->name);
		}
		free(symbol_info);
	}
	if (provider_info != NULL && provider_info->usage_count == 0) {
		assert(provider_info->symbols == NULL || hash_isempty(provider_info->symbols));
		if ((node = hash_lookup(context->symbol_providers, pp)) != NULL) {
			hash_delete_free(context->symbol_providers, node);
		}
		free(provider_info);
	}

	// Report insufficient memory error
	if (status == CP_ERR_RESOURCE && !error_reported) {
		cpi_errorf(context, _("Symbol %s in plug-in %s could not be resolved due to insufficient memory."), name, id);
	}

	// Return error code
	if (error != NULL) {
		*error = status;
	}
	
	// Return symbol
	return symbol;
}

void CP_API cp_release_symbol(cp_context_t *context, void *ptr) {
	
	assert(context != NULL);
	assert(ptr != NULL);
	
	// TODO
}
