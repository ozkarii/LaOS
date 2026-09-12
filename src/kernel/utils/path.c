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
  if (path[0] != '/') {
    // Has to be absolute path
    return false;
  }
  
  int i = 0;
  while (path[i]) {
    char c = path[i];
    // Has to be proper character: a-z A-Z 0-9 / . _ -
    if (!((c >= 'a' && c <= 'z') ||
          (c >= 'A' && c <= 'Z') ||
          (c >= '0' && c <= '9') ||
          c == '/' || c == '.' || c == '_' || c == '-')) {
      return false;
    }

    if (c == '/' && path[i + 1] == '/') {
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
  const char* path_without_mount_point;
  if (!strcmp(mount_point, "/")) {
    // Special case if mount is root
    path_without_mount_point = path;
  }
  else {
    path_without_mount_point = strip_prefix(path, mount_point);
  }

  return path_without_mount_point;
}

char* strip_prefix(const char* src, const char* prefix) {
  int i = 0; // Where to move the pointer to
  while (src[i] && prefix[i]) {
    if (src[i] != prefix[i]) {
      // Mismatch, return original string
      return (char*)src;
    }
    i++;
  }
  return (char*)(src + i);
}


int calculate_common_path_depth(const char* path, const char* mount_point) {
  // Special case for root mount point
  if (mount_point[0] == '/' && mount_point[1] == '\0') {
    return 0;
  }

  int depth = 0;
  int i = 0;
  while (path[i] != '\0' && mount_point[i] != '\0') {
    if (path[i] != mount_point[i]) {
      // Mismatch mound, not a valid mount point for this path
      return -1;
    }
    if (path[i] == '/') {
      // Slash found, increment depth
      depth++;
    }

    // Next character
    i++;
  }

  return depth;
}