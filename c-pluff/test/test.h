#ifndef TEST_H_
#define TEST_H_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <cpluff.h>

// GNU C attribute defines
#ifndef CP_GCC_NORETURN
#if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 5)
#define CP_GCC_NORETURN __attribute__((noreturn))
#else
#define CP_GCC_NORETURN
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Prints failure message and aborts the test program.
 * 
 * @param func the test function
 * @param file the test source file
 * @param line the test source line
 * @param msg the failure message
 */
CP_HIDDEN void fail(const char *func, const char *file, int line, const char *msg) CP_GCC_NORETURN CP_GCC_NONNULL(1, 2, 4);

/**
 * Checks that the specified condition is true.
 * 
 * @param cond the condition that should be true
 */
#define check(cond) do { if (!(cond)) { fail(__func__, __FILE__, __LINE__, "Failed condition: " #cond); }} while (0)

/**
 * Returns the plug-in path for the specified test plug-in.
 * The returned string is valid until the next call to plugindir.
 * 
 * @return plug-in path for the specified test plug-in
 */
CP_HIDDEN const char *plugindir(const char *plugin) CP_GCC_NONNULL(1);

/**
 * Returns the plug-in collection path for the specified test collection.
 * The returned string is valid until the next call to pcollectiondir.
 * 
 * @return plug-in collection path for the specified test collection
 */
CP_HIDDEN const char *pcollectiondir(const char *collection) CP_GCC_NONNULL(1);

/**
 * Initializes the C-Pluff framework and creates a plug-in context.
 * Checks for any failures on the way. Also prints out context errors/warnings
 * and maintains a count of logged context errors if so requested.
 *
 * @param min_disp_sev the minimum severity of messages to be displayed
 * @param error_counter pointer to the location where the logged error count is to be stored or NULL 
 * @return the created plug-in context
 */
CP_HIDDEN cp_context_t *init_context(cp_log_severity_t min_disp_sev, int *error_counter);

/**
 * Frees any test resources. This can be called to ensure there are no memory
 * leaks due to leaked test resources.
 */
CP_HIDDEN void free_test_resources(void);

#ifdef __cplusplus
}
#endif

#endif /*TEST_H_*/
