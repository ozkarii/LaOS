#include <stddef.h>
#include "string.h"
#include "vfs.h"

// Singleton instance of the VFS
VFS *vfs_instance = NULL;

static VFSMountPoint* find_mount_point(VFSMountPoint* mounts, const char* path) {
  VFSMountPoint* winner = NULL;
  int winner_depth = -1;

  for (unsigned i = 0; i < MAX_MOUNTS; i++) {
    VFSMountPoint* mount = &mounts[i];
    if (!mount->active) {
      continue;
    }

    int depth = calculate_first_diff_depth(path, mount->path);

    if (depth > winner_depth) {
      winner_depth = depth;
      winner = mount;
    }
  }

  return winner;
}

static bool is_already_mountpoint(VFSMountPoint mounts[MAX_MOUNTS], const char* path) {
  for (unsigned i = 0; i < MAX_MOUNTS; i++) {
    VFSMountPoint* mount = &mounts[i];
    if (!mount->active) {
      continue;
    }

    if (!strcmp(mount->path, path)) {
      return true;
    }
  }
  return false;
}


int vfs_init(VFS* vfs) {
  if (vfs == NULL) {
    return -1;
  }
  if (vfs_instance != NULL && vfs_instance->initialized) {
    return -1;  // Already initialized
  }

  memset(vfs, 0, sizeof(VFS));
  vfs->initialized = true;
  vfs_instance = vfs;
  return 0;
}

int vfs_mount(const char *path, VFSInterface *fs, void *fs_data) {
  if (!is_valid_path(path)) {
    return -1;
  }

  if (is_already_mountpoint(vfs_instance->mounts, path)) {
    return -1;  // Cannot mount multiple times to same path
  }

  for (unsigned i = 0; i < MAX_MOUNTS; i++) {
    VFSMountPoint* mount = &vfs_instance->mounts[i];
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


VFSFileDescriptor* vfs_open(const char *path, int flags, mode_t mode) {
  if (!is_valid_path(path)) {
    return NULL;
  }

  // Find free slot
  int free_fd_slot = -1;
  for (unsigned i = 0; i < MAX_OPEN_FILES; i++) {
    if (!vfs_instance->open_files[i].allocated) {
      free_fd_slot = i;
      break;
    }
  }
  if (free_fd_slot == -1) {
    return NULL;
  }

  VFSMountPoint* mount_point = find_mount_point(vfs_instance->mounts, path);
  if (mount_point == NULL) {
    return NULL;
  }

  const char* path_without_mount_point 
    = get_path_without_mount_point(path, mount_point->path);

  void* file_handle = mount_point->fs->open(mount_point->fs_data,
                                       path_without_mount_point,
                                       flags, mode);
  
  if (file_handle == NULL) {
    return NULL;
  }

  VFSFileDescriptor* vfs_fd = &vfs_instance->open_files[free_fd_slot];
  vfs_fd->allocated = true;
  vfs_fd->ref_count++;
  vfs_fd->mount = mount_point;
  vfs_fd->opaque_file_handle = file_handle;
  vfs_fd->mode = mode;

  return vfs_fd;
}


ssize_t vfs_read(VFSFileDescriptor* fd, void *buffer, size_t size) {
  return fd->mount->fs->read(fd->mount->fs_data, fd->opaque_file_handle, buffer, size);
}

ssize_t vfs_write(VFSFileDescriptor* fd, const void *buffer, size_t size) {
  return fd->mount->fs->write(fd->mount->fs_data, fd->opaque_file_handle, buffer, size);
}

int vfs_close(VFSFileDescriptor* fd) {
  if (fd == NULL || !fd->allocated) {
    return -1;
  }

  fd->ref_count--;

  if (fd->ref_count > 0) {
    return 0; // Still in use
  }

  int res = fd->mount->fs->close(fd->mount->fs_data, fd->opaque_file_handle);
  fd->allocated = false;
  fd->opaque_file_handle = NULL;
  return res;
}

int vfs_seek(VFSFileDescriptor* fd, size_t offset) {
  return fd->mount->fs->seek(fd->mount->fs_data, fd->opaque_file_handle, offset);
}

int vfs_mkdir(const char* path) {
  VFSMountPoint* mount = find_mount_point(vfs_instance->mounts, path);
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

int vfs_remove(const char* path) {
  VFSMountPoint* mount = find_mount_point(vfs_instance->mounts, path);
  if (mount == NULL) {
    return -1;
  }

  const char* path_without_mount_point 
    = get_path_without_mount_point(path, mount->path);
  
  return mount->fs->remove(mount->fs_data, path_without_mount_point);
}

int vfs_stat(const char* path, VFSStat* stat) {
  VFSMountPoint* mount = find_mount_point(vfs_instance->mounts, path);
  if (mount == NULL) {
    return -1;
  }

  const char* path_without_mount_point 
    = get_path_without_mount_point(path, mount->path);
  
  return mount->fs->stat(mount->fs_data, path_without_mount_point, stat);
}