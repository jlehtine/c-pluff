/*-------------------------------------------------------------------------
 * C-Pluff, a plug-in framework for C
 * Copyright 2006 Johannes Lehtinen
 *-----------------------------------------------------------------------*/

/*
 * Dynamic plug-in symbols
 */

#include <stdlib.h>
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

CP_API int cp_define_symbol(cp_context_t *context, const char *name, void *ptr) {
	int status = CP_OK;
	
	CHECK_NOT_NULL(context);
	CHECK_NOT_NULL(name);
	CHECK_NOT_NULL(ptr);
	if (context->plugin == NULL) {
		cpi_fatalf(_("Only plug-ins can define context specific symbols."));
	}
	
	cpi_lock_context(context);
	cpi_check_invocation(context, CPI_CF_LOGGER | CPI_CF_LISTENER, __func__);
	do {
		char *n;
		
		// Create a symbol hash if necessary
		if (context->plugin->defined_symbols == NULL) {
			if ((context->plugin->defined_symbols = hash_create(HASHCOUNT_T_MAX, (int (*)(const void *, const void *)) strcmp, NULL)) == NULL) {
				status = CP_ERR_RESOURCE;
				break;
			}
		}
		
		// Check for a previously defined symbol
		if (hash_lookup(context->plugin->defined_symbols, name) != NULL) {
			status = CP_ERR_CONFLICT;
			break;
		}

		// Insert the symbol into the symbol hash
		n = cpi_strdup(name);
		if (n == NULL || !hash_alloc_insert(context->plugin->defined_symbols, name, ptr)) {
			free(n);
			status = CP_ERR_RESOURCE;
			break;
		} 

	} while (0);
	cpi_unlock_context(context);
	
	// Report error
	if (status != CP_OK) {
		switch (status) {
			case CP_ERR_RESOURCE:
				cpi_errorf(context, _("Plug-in %s could not define symbol %s due to insufficient memory."), context->plugin->plugin->identifier, name);
				break;
			case CP_ERR_CONFLICT:
				cpi_errorf(context, _("Plug-in %s tried to redefine symbol %s."), context->plugin->plugin->identifier, name);
				break;
		}
	}
	
	return status;
}

CP_API void * cp_resolve_symbol(cp_context_t *context, const char *id, const char *name, int *error) {
	int status = CP_OK;
	int error_reported = 1;
	hnode_t *node;
	void *symbol = NULL;
	symbol_info_t *symbol_info = NULL;
	symbol_provider_info_t *provider_info = NULL;
	cp_plugin_t *pp = NULL;

	CHECK_NOT_NULL(context);
	CHECK_NOT_NULL(id);
	CHECK_NOT_NULL(name);
	if (context->plugin == NULL) {
		cpi_fatalf(_("Only plug-ins can resolve dynamic symbols."));
	}
	
	// Resolve the symbol
	cpi_lock_context(context);
	cpi_check_invocation(context, CPI_CF_LOGGER | CPI_CF_LISTENER | CPI_CF_STOP, __func__);
	do {

		// Allocate space for symbol hashes, if necessary
		if (context->plugin->resolved_symbols == NULL) {
			context->plugin->resolved_symbols = hash_create(HASHCOUNT_T_MAX, cpi_comp_ptr, cpi_hashfunc_ptr);
		}
		if (context->plugin->symbol_providers == NULL) {
			context->plugin->symbol_providers = hash_create(HASHCOUNT_T_MAX, cpi_comp_ptr, cpi_hashfunc_ptr);
		}
		if (context->plugin->resolved_symbols == NULL
			|| context->plugin->symbol_providers == NULL) {
			status = CP_ERR_RESOURCE;
			break;
		}

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

		// Check for a context specific symbol
		if (pp->defined_symbols != NULL && (node = hash_lookup(pp->defined_symbols, name)) != NULL) {
			symbol = hnode_get(node);
		}

		// Fall back to global symbols, if necessary
		if (symbol == NULL && pp->runtime_lib != NULL) {
			symbol = DLSYM(pp->runtime_lib, name);
		}
		if (symbol == NULL) {
			cpi_warnf(context, _("Symbol %s in plug-in %s could not be resolved because it is not defined."), name, id);
			status = CP_ERR_UNKNOWN;
			break;
		}

		// Lookup or initialize symbol provider information
		if ((node = hash_lookup(context->plugin->symbol_providers, pp)) != NULL) {
			provider_info = hnode_get(node);
		} else {
			if ((provider_info = malloc(sizeof(symbol_provider_info_t))) == NULL) {
				status = CP_ERR_RESOURCE;
				break;
			}
			memset(provider_info, 0, sizeof(symbol_provider_info_t));
			provider_info->plugin = pp;
			provider_info->imported = cpi_ptrset_contains(context->plugin->imported, pp);
			if (!hash_alloc_insert(context->plugin->symbol_providers, pp, provider_info)) {
				status = CP_ERR_RESOURCE;
				break;
			}
		}
		
		// Lookup or initialize symbol information
		if ((node = hash_lookup(context->plugin->resolved_symbols, symbol)) != NULL) {
			symbol_info = hnode_get(node);
		} else {
			if ((symbol_info = malloc(sizeof(symbol_info_t))) == NULL) {
				status = CP_ERR_RESOURCE;
				break;
			}
			memset(symbol_info, 0, sizeof(symbol_info_t));
			symbol_info->provider_info = provider_info;
			if (!hash_alloc_insert(context->plugin->resolved_symbols, symbol, symbol_info)) {
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
			cpi_debugf(context, _("A dynamic dependency was created from plug-in %s to plug-in %s."), context->plugin->plugin->identifier, pp->plugin->identifier);
		}
		
		// Increase usage counts
		symbol_info->usage_count++;
		provider_info->usage_count++;

	} while (0);

	// Clean up
	if (symbol_info != NULL && symbol_info->usage_count == 0) {
		if ((node = hash_lookup(context->plugin->resolved_symbols, symbol)) != NULL) {
			hash_delete_free(context->plugin->resolved_symbols, node);
		}
		free(symbol_info);
	}
	if (provider_info != NULL && provider_info->usage_count == 0) {
		if ((node = hash_lookup(context->plugin->symbol_providers, pp)) != NULL) {
			hash_delete_free(context->plugin->symbol_providers, node);
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

CP_API void cp_release_symbol(cp_context_t *context, const void *ptr) {
	hnode_t *node;
	symbol_info_t *symbol_info;
	symbol_provider_info_t *provider_info;
	
	CHECK_NOT_NULL(context);
	CHECK_NOT_NULL(ptr);
	if (context->plugin == NULL) {
		cpi_fatalf(_("Only plug-ins can use dynamic symbols."));
	}

	cpi_lock_context(context);
	cpi_check_invocation(context, CPI_CF_LOGGER | CPI_CF_LISTENER, __func__);
	do {

		// Look up the symbol
		if ((node = hash_lookup(context->plugin->resolved_symbols, ptr)) == NULL) {
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
			hash_delete_free(context->plugin->resolved_symbols, node);
			free(symbol_info);
			cpi_debugf(context, _("Plug-in %s released symbol %p defined by plug-in %s."), context->plugin->plugin->identifier, ptr, provider_info->plugin->plugin->identifier);
		}
	
		// Check if the symbol providing plug-in is not being used anymore
		if (provider_info->usage_count == 0) {
			node = hash_lookup(context->plugin->symbol_providers, provider_info->plugin);
			assert(node != NULL);
			hash_delete_free(context->plugin->symbol_providers, node);
			if (!provider_info->imported) {
				cpi_ptrset_remove(context->plugin->imported, provider_info->plugin);
				cpi_ptrset_remove(provider_info->plugin->importing, context->plugin);
			}
			cpi_debugf(context, _("A dynamic dependency from plug-in %s to plug-in %s was removed."), context->plugin->plugin->identifier, provider_info->plugin->plugin->identifier);
			free(provider_info);
		}
		
	} while (0);
	cpi_unlock_context(context);
}
