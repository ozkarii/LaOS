#ifndef PATH_H
#define PATH_H

#include <stdbool.h>

#define PATH_NAME_MAX 64
#define PATH_SEPARATOR '/'

bool is_valid_path(const char* path);
const char* get_path_without_mount_point(const char* path, const char* mount_point);
char* strip_prefix(const char* src, const char* prefix);

// Returns the depth of the first difference between two paths, or -1 if they are identical.
int calculate_first_diff_depth(const char* path1, const char* path2);

#endif // PATH_H