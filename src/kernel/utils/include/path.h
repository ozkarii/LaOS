#ifndef PATH_H
#define PATH_H

#include <stdbool.h>

#include "kernel_utils_autoconf.h"

#define PATH_NAME_MAX CONFIG_UTILS_PATH_NAME_MAX

// All other functions in this layer assume valid paths
// Use this function to validate paths before passing them to other functions
bool is_valid_path(const char* path);


const char* get_path_without_mount_point(const char* path, const char* mount_point);
char* strip_prefix(const char* src, const char* prefix);

/**
 * @brief Calculates the depth of the common path prefix between two paths.
 * Used for determining the mount point that best matches a given path.
 * Assumes both paths are valid absolute paths.
 * @param path The full path to check.
 * @param mount_point The mount point path.
 * @return The depth of the common path prefix, or -1 if no common prefix exists.
 */
int calculate_common_path_depth(const char* path, const char* mount_point);

// Resolves a relative path against a current working directory (cwd) and outputs the absolute path.
// The output buffer `out_abs_path` must be at least PATH_NAME_MAX in size.
// Input `cwd` and `relative_path` must be null-terminated strings and valid paths.
// Returns 0 on success, -1 if the resulting path exceeds PATH_NAME_MAX
int resolve_relative_path(const char* cwd, const char* relative_path, char* out_abs_path);

#endif // PATH_H