#ifndef PATH_H
#define PATH_H

#include <stdbool.h>

#define PATH_NAME_MAX 64

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

#endif // PATH_H