#ifndef VFS_H
#define VFS_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define NAME_MAX 64
#define MAX_OPEN_FILES 64
#define MAX_MOUNTS 16
#define PATH_SEPARATOR '/'

#define MODE_READ   0x1
#define MODE_WRITE  0x2
#define MODE_APPEND 0x4
#define MODE_CREATE 0x8


typedef struct VFSStat {
  size_t size;
  bool is_directory;
} VFSStat;

typedef struct VFSInterface {
  void* (*open)(void* fs_data, const char* path, unsigned mode);
  size_t (*read)(void* fs_data, void* file, void* buffer, size_t size);
  int (*write)(void* fs_data, void* file, const void* buffer, size_t size);
  int (*close)(void* fs_data, void* file);
  int (*seek)(void* fs_data, void* file, size_t offset);
  int (*mkdir)(void* fs_data, const char* path);
  int (*readdir)(void* fs_data, void* dir, char* buffer, size_t size);
  int (*remove)(void* fs_data, const char* path);
  int (*stat)(void* fs_data, const char* path, VFSStat* stat);
} VFSInterface;

typedef struct VFSMountPoint {
  char path[NAME_MAX];
  VFSInterface* fs;
  void* fs_data;
  bool active;
} VFSMountPoint;

typedef struct VFSFileDescriptor {
  bool in_use;
  VFSMountPoint* mount;
  void* opaque_file_handle;
  unsigned mode;
} VFSFileDescriptor;

int vfs_mount(const char* path, VFSInterface* fs, void* fs_data);
VFSFileDescriptor* vfs_open(const char* path, unsigned mode);
size_t vfs_read(VFSFileDescriptor* fd, void* buffer, size_t size);
int vfs_write(VFSFileDescriptor* fd, const void* buffer, size_t size);
int vfs_close(VFSFileDescriptor* fd);
int vfs_seek(VFSFileDescriptor* fd, size_t offset);
int vfs_mkdir(const char* path);
int vfs_readdir(VFSFileDescriptor* fd, char* buffer, size_t size);
int vfs_remove(const char* path);
int vfs_stat(const char* path, VFSStat* stat);

#endif // VFS_H