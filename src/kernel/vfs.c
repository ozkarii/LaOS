#include <stddef.h>
#include "string.h"
#include "vfs.h"


VFSMountPoint mounts[MAX_MOUNTS];
VFSFileDescriptor open_files[MAX_OPEN_FILES];


static int first_diff_depth(const char* path1, const char* path2) {
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

static VFSMountPoint* find_mount_point(const char* path) {
  VFSMountPoint* winner = NULL;
  int winner_depth = -1;

  for (unsigned i = 0; i < MAX_MOUNTS; i++) {
    VFSMountPoint* mount = &mounts[i];

    if (!mount->active) {
      continue;
    }

    int depth = first_diff_depth(path, mount->path);

    if (depth > winner_depth) {
      winner_depth = depth;
      winner = mount;
    }
  }

  return winner;
}

static char* strip_prefix(const char* src, const char* prefix) {
  char* src_tmp = (char*)src;
  char* prefix_tmp = (char*)prefix;

  while (*src_tmp == *prefix_tmp && *src_tmp) {
    src_tmp++;
    prefix_tmp++;
  }

  return src_tmp;
}

static const char* 
get_path_without_mount_point(const char* path, const char* mount_point) {
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

static bool is_mountpoint(const char* path) {
  for (unsigned i = 0; i < MAX_MOUNTS; i++) {
    if (strcmp(mounts[i].path, path)) {
      return false;
    }
  }
  return true;
}

static bool is_valid_path(const char* path) {
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

    if (i > NAME_MAX) {
      // Too long
      return false;
    }
  }
  
  return true;
}

int vfs_mount(const char *path, VFSInterface *fs, void *fs_data) {
  if (!is_valid_path(path)) {
    return -1;
  }

  if (is_mountpoint(path)) {
    return -1;  // Cannot mount multiple times to same path
  }

  for (unsigned i = 0; i < MAX_MOUNTS; i++) {
    VFSMountPoint* mount = &mounts[i];
    if (mount->active) {
      continue;
    }

    strcpy(mount->path, path);
    mount->fs = fs;
    mount->fs_data = fs_data;
    mount->active = true;
    return 0;
  }

  return -1;
}


VFSFileDescriptor* vfs_open(const char *path, unsigned mode) {
  if (!is_valid_path(path)) {
    return NULL;
  }

  // Find free slot
  int free_fd_slot = -1;
  for (unsigned i = 0; i < MAX_OPEN_FILES; i++) {
    if (!open_files[i].in_use) {
      free_fd_slot = i;
      break;
    }
  }
  if (free_fd_slot == -1) {
    return NULL;
  }

  VFSMountPoint* mount_point = find_mount_point(path);
  if (mount_point == NULL) {
    return NULL;
  }

  const char* path_without_mount_point 
    = get_path_without_mount_point(path, mount_point->path);

  void* file_handle = mount_point->fs->open(mount_point->fs_data,
                                       path_without_mount_point,
                                       mode);
  
  if (file_handle == NULL) {
    return NULL;
  }

  VFSFileDescriptor* vfs_fd = &open_files[free_fd_slot];
  vfs_fd->in_use = true;
  vfs_fd->mount = mount_point;
  vfs_fd->opaque_file_handle = file_handle;
  vfs_fd->mode = mode;

  return vfs_fd;
}


int vfs_read(VFSFileDescriptor* fd, void *buffer, size_t size) {
  return fd->mount->fs->read(fd->mount->fs_data, fd->opaque_file_handle, buffer, size);
}

int vfs_write(VFSFileDescriptor* fd, const void *buffer, size_t size) {
  return fd->mount->fs->write(fd->mount->fs_data, fd->opaque_file_handle, buffer, size);
}

int vfs_close(VFSFileDescriptor* fd) {
  int res = fd->mount->fs->close(fd->mount->fs_data, fd->opaque_file_handle);
  fd->in_use = false;
  fd->opaque_file_handle = NULL;
  return res;
}

int vfs_seek(VFSFileDescriptor* fd, size_t offset) {
  return fd->mount->fs->seek(fd->mount->fs_data, fd->opaque_file_handle, offset);
}

int vfs_mkdir(const char* path) {
  VFSMountPoint* mount = find_mount_point(path);
  if (mount == NULL) {
    return -1;
  }

  const char* path_without_mount_point 
    = get_path_without_mount_point(path, mount->path);

  return mount->fs->mkdir(mount->fs_data, path_without_mount_point);
}

int vfs_readdir(VFSFileDescriptor* fd, char* buffer, size_t size) {
  return fd->mount->fs->readdir(fd->mount->fs_data, fd->opaque_file_handle, buffer, size);
}