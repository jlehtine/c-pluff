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
	
	// The providing plug-in
	cp_plugin_t *plugin;
	
	// Whether there is also an import dependency for the plug-in
	int imported;
	
	// Total symbol usage count
	int usage_count;
	
} symbol_provider_info_t;

/// Information about used symbol
typedef struct symbol_info_t {
	
	// Symbol usage count
	int usage_count;
	
	// Information about providing plug-in
	symbol_provider_info_t *provider_info;
	
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
			provider_info->plugin = pp;
			provider_info->imported = cpi_ptrset_contains(context->plugin->imported, pp);
			if (!hash_alloc_insert(context->symbol_providers, pp, provider_info)) {
				status = CP_ERR_RESOURCE;
				break;
			}
		}
		
		// Lookup or initialize symbol information
		if ((node = hash_lookup(context->symbols, symbol)) != NULL) {
			symbol_info = hnode_get(node);
		} else {
			if ((symbol_info = malloc(sizeof(symbol_info_t))) == NULL) {
				status = CP_ERR_RESOURCE;
				break;
			}
			memset(symbol_info, 0, sizeof(symbol_info_t));
			symbol_info->provider_info = provider_info;
			if (!hash_alloc_insert(context->symbols, symbol, symbol_info)) {
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

	// Clean up
	if (symbol_info != NULL && symbol_info->usage_count == 0) {
		if ((node = hash_lookup(context->symbols, symbol)) != NULL) {
			hash_delete_free(context->symbols, node);
		}
		free(symbol_info);
	}
	if (provider_info != NULL && provider_info->usage_count == 0) {
		if ((node = hash_lookup(context->symbol_providers, pp)) != NULL) {
			hash_delete_free(context->symbol_providers, node);
		}
		free(provider_info);
	}
	cpi_unlock_context(context);

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
	hnode_t *node;
	symbol_info_t *symbol_info;
	symbol_provider_info_t *provider_info;
	
	assert(context != NULL);
	assert(ptr != NULL);

	cpi_lock_context(context);
	do {

		// Look up the symbol
		if ((node = hash_lookup(context->symbols, ptr)) == NULL) {
			cpi_errorf(context, _("Could not release an unknown symbol %p."), ptr);
			break;
		}
		symbol_info = hnode_get(node);
		provider_info = symbol_info->provider_info;
	
		// Decrease usage count
		assert(symbol_info->usage_count > 0);
		assert(provider_info->usage_count > 0);
		symbol_info->usage_count--;
		provider_info->usage_count--;
	
		// Check if the symbol is not being used anymore
		if (symbol_info->usage_count == 0) {
			hash_delete_free(context->symbols, node);
			free(symbol_info);
		}
	
		// Check if the symbol providing plug-in is not being used anymore
		if (provider_info->usage_count == 0) {
			node = hash_lookup(context->symbol_providers, provider_info->plugin);
			assert(node != NULL);
			hash_delete_free(context->symbol_providers, node);
			if (!provider_info->imported) {
				cpi_ptrset_remove(context->plugin->imported, provider_info->plugin);
				cpi_ptrset_remove(provider_info->plugin->importing, context->plugin);
			}
			free(provider_info);
		}
		
	} while (0);
	cpi_unlock_context(context);
}
