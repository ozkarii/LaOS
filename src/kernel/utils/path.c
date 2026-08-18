#include "string.h"

#include "path.h"

bool is_valid_path(const char* path) {
  if (!path) {
    // No NULL pointers
    return false;
  }
  if (path[0] == '\0') {
    // No empty paths
    return false;
  }
  if (path[0] != PATH_SEPARATOR) {
    // Has to be absolute path
    return false;
  }
  
  int i = 0;
  while (path[i]) {
    char c = path[i];
    // Has to be proper character: a-z A-Z 0-9 / . _ -
    if (c < 45 || (c > 57 && c < 65) || (c > 90 && c < 97) || c > 122) {
      return false;
    }

    if (c == PATH_SEPARATOR && path[i + 1] == PATH_SEPARATOR) {
      // No double slashes
      return false;
    }
  
    i++;

    if (i > PATH_NAME_MAX) {
      // Too long
      return false;
    }
  }
  
  return true;
}

const char* get_path_without_mount_point(const char* path, const char* mount_point) {
  static const char root_str[2] = {PATH_SEPARATOR, '\0'};

  const char* path_without_mount_point;
  if (!strcmp(mount_point, root_str)) {
    // Special case if mount is root
    path_without_mount_point = path;
  }
  else {
    path_without_mount_point = strip_prefix(path, mount_point);
  }

  return path_without_mount_point;
}

char* strip_prefix(const char* src, const char* prefix) {
  char* src_tmp = (char*)src;
  char* prefix_tmp = (char*)prefix;

  while (*src_tmp == *prefix_tmp && *src_tmp) {
    src_tmp++;
    prefix_tmp++;
  }

  return src_tmp;
}

int calculate_first_diff_depth(const char* path1, const char* path2) {
  int depth = -1;
  char* tmp_path = (char*)path1;
  char* tmp_mnt_path = (char*)path2;

  while (*tmp_path && *tmp_mnt_path && (*tmp_path == *tmp_mnt_path)) {
    if (*tmp_path == PATH_SEPARATOR) {
      depth++;
    }
    tmp_path++;
    tmp_mnt_path++;
  }
  return depth;
}