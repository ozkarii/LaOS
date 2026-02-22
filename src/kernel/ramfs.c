#include "string.h"
#include "vfs.h"

#define RAMFS_MAX_FILES 64
#define RAMFS_MAX_CHILDREN 16
#define RAMFS_MAX_FILE_SIZE (1024 * 4)  // 4KB max file size
#define ROOT_IDX 0  // Root should always be at index 0 in files array

typedef struct RamFSFile RamFSFile;

typedef struct RamFSFile {
  char path[NAME_MAX];
  uint8_t data[RAMFS_MAX_FILE_SIZE];
  size_t size;
  bool is_directory;
  bool is_allocated;
  RamFSFile* parent;
  RamFSFile* children[RAMFS_MAX_CHILDREN];
} RamFSFile;

typedef struct RamFSHandle {
  RamFSFile *file;
  size_t offset;
} RamFSHandle;

typedef struct RamFS {
  RamFSFile files[RAMFS_MAX_FILES];
  RamFSHandle handles[RAMFS_MAX_FILES];
} RamFS;


static void* ramfs_open(void *fs_data, const char *path, unsigned mode);
static size_t ramfs_read(void *fs_data, void *file, void *buffer, size_t size);
static int ramfs_write(void *fs_data, void *file, const void *buffer, size_t size);
static int ramfs_close(void *fs_data, void *file);
static int ramfs_seek(void *fs_data, void *file, size_t offset);
static int ramfs_mkdir(void* fs_data, const char* path);
static int ramfs_readdir(void* fs_data, void* handle, char* buffer, size_t size);
static int ramfs_remove(void* fs_data, const char* path);
static int ramfs_stat(void* fs_data, const char* path, VFSStat* stat);

VFSInterface ramfs_if = {
  .open = ramfs_open,
  .read = ramfs_read,
  .write = ramfs_write,
  .close = ramfs_close,
  .seek = ramfs_seek,
  .mkdir = ramfs_mkdir,
  .readdir = ramfs_readdir,
  .remove = ramfs_remove,
  .stat = ramfs_stat
};

void* ramfs_init(void* dest, size_t max_size) {
  if (max_size < sizeof(RamFS)) {
    return NULL;
  }

  // Initialize ramfs to the provided buffer
  memset(dest, 0, sizeof(RamFS));
  RamFS* fs = (RamFS*)dest;

  // Create root directory
  fs->files[ROOT_IDX].is_allocated = true;
  fs->files[ROOT_IDX].is_directory = true;
  fs->files[ROOT_IDX].path[0] = PATH_SEPARATOR;
  
  return fs;
}

VFSInterface* ramfs_get_vfs_interface(void) {
  return &ramfs_if;
}

size_t ramfs_get_size(void) {
  return sizeof(RamFS);
}

static RamFSFile* find_file(RamFS *fs, const char *path) {
  for (int i = 0; i < RAMFS_MAX_FILES; i++) {
    if (fs->files[i].is_allocated && strcmp(fs->files[i].path, path) == 0) {
      return &fs->files[i];
    }
  }
  return NULL;
}

static RamFSFile* find_parent(RamFS *fs, const char *path) {
  // Find last separator to get parent path
  const char *last_sep = path;
  for (const char* p = path; *p; p++) {
    if (*p == PATH_SEPARATOR) {
      last_sep = p;
    }
  }

  if (last_sep == path) {
    // Parent is root
    return &fs->files[ROOT_IDX];
  }

  // Build parent path
  char parent_path[NAME_MAX];
  size_t len = last_sep - path;
  memcpy(parent_path, path, len);
  parent_path[len] = '\0';

  return find_file(fs, parent_path);
}

static RamFSFile* create_file(RamFS *fs, const char *path, bool is_dir) {
  // Find parent directory first
  RamFSFile *parent = find_parent(fs, path);
  if (!parent || !parent->is_directory) {
    return NULL;
  }

  int child_slot_in_parent = -1;
  for (int i = 0; i < RAMFS_MAX_CHILDREN; i++) {
    if (parent->children[i] == NULL) {
      child_slot_in_parent = i;
    }
  }
  if (child_slot_in_parent == -1) {
    return NULL;
  }

  // Allocate file
  for (int i = 0; i < RAMFS_MAX_FILES; i++) {
    if (!fs->files[i].is_allocated) {
      RamFSFile* file = &fs->files[i];
      file->is_allocated = true;
      file->is_directory = is_dir;
      file->size = 0;
      strcpy(file->path, path);
      file->parent = parent;
      parent->children[child_slot_in_parent] = file;

      return file;
    }
  }

  return NULL;
}

static int destroy_file(RamFSFile* file) {
  if (file->parent == NULL) {
    return -1; // Can't, if root
  }

  if (file->is_directory) {
    for (int i = 0; i < RAMFS_MAX_CHILDREN; i++) {
      if (file->children[i] != NULL) {
        return -1; // Can't destroy dir containing files
      }
    }
  }

  for (int i = 0; i < RAMFS_MAX_CHILDREN; i++) {
    if (file->parent->children[i] == file) {
      file->parent->children[i] = NULL;
    }
  }

  memset(file, 0, sizeof(RamFSFile));

  return 0;
}

static RamFSHandle* allocate_handle(RamFS* fs, RamFSFile* file) {
  RamFSHandle* handle = NULL;
  for (int i = 0; i < MAX_OPEN_FILES; i++) {
    if (fs->handles[i].file == NULL) {
      handle = &fs->handles[i];
      handle->file = file;
      handle->offset = 0;
      return handle;
    }
  }
  return NULL;
}

static void* ramfs_open(void *fs_data, const char *path, unsigned mode) {
  RamFS *fs = (RamFS*)fs_data;
  RamFSFile *file = find_file(fs, path);

  if ((file == NULL) && (mode & MODE_CREATE)) {
    file = create_file(fs, path, false);
  }
  if (file == NULL) {
    return NULL;
  }

  RamFSHandle *handle = allocate_handle(fs, file);
  if (handle == NULL) {
    destroy_file(file);
  }

  return handle;
}

static size_t ramfs_read(void *fs_data, void *handle, void *buffer, size_t size) {
  (void)fs_data;
  RamFSHandle *h = (RamFSHandle*)handle;
  
  size_t remaining = h->file->size - h->offset;
  size_t to_read = (size < remaining) ? size : remaining;
  
  memcpy(buffer, h->file->data + h->offset, to_read);
  h->offset += to_read;
  
  return to_read;
}

static int ramfs_write(void *fs_data, void *handle, const void *buffer, size_t size) {
  (void)fs_data;
  RamFSHandle *h = (RamFSHandle*)handle;
  
  size_t space = RAMFS_MAX_FILE_SIZE - h->offset;
  size_t to_write = (size < space) ? size : space;
  
  memcpy(h->file->data + h->offset, buffer, to_write);
  h->offset += to_write;
  
  if (h->offset > h->file->size) {
    h->file->size = h->offset;
  }
  
  return (int)to_write;
}

static int ramfs_close(void *fs_data, void *handle) {
  (void)fs_data;
  RamFSHandle *h = (RamFSHandle*)handle;
  memset(h, 0, sizeof(RamFSHandle));
  return 0;
}

static int ramfs_seek(void *fs_data, void *handle, size_t offset) {
  (void)fs_data;
  RamFSHandle *h = (RamFSHandle*)handle;
  
  if (offset > h->file->size) {
    return -1;  // Can't seek past end
  }
  
  h->offset = offset;
  return 0;
}

static int ramfs_mkdir(void* fs_data, const char* path) {
  RamFS *fs = (RamFS*)fs_data;
  RamFSFile *file = find_file(fs, path);
  
  if (!file) {
    file = create_file(fs, path, true);
  }

  if (!file) {
    return -1;
  }

  return 0;
}

static int ramfs_readdir(void* fs_data, void* handle, char* buffer, size_t size) {
  (void)fs_data;
  RamFSHandle *h = (RamFSHandle*)handle;

  if (!h->file->is_directory) {
    return -1;
  }

  size_t written = 0;

  for (unsigned i = 0; i < RAMFS_MAX_CHILDREN; i++) {
    RamFSFile* child = h->file->children[i];
    if (child && child->is_allocated) {
      // Extract just the filename from the full path
      const char* name = child->path;
      for (const char *p = child->path; *p; p++) {
        if (*p == PATH_SEPARATOR && *(p + 1)) {
          name = p + 1;
        }
      }

      // Output is just a list of filenames
      size_t name_len = strlen(name);
      if ((written + name_len + 1) < size) {
        strcpy(buffer + written, name);
        written += name_len;
        buffer[written++] = ' ';
      }
    }
  }

  if (written > 0) {
    buffer[written - 1] = '\0';  // Replace last space with null
  } else {
    buffer[0] = '\0';
  }

  return 0;
}

static int ramfs_remove(void* fs_data, const char* path) {
  RamFS* fs = (RamFS*)fs_data;

  RamFSFile* file = find_file(fs_data, path);
  if (file == NULL) {
    return -1; // File doesn't exist
  }

  for (int i = 0; i < MAX_OPEN_FILES; i++) {
    if (fs->handles[i].file == file) {
      return -1; // Can't, if someone has opened this file
    }
  }

  return destroy_file(file);
}

static int ramfs_stat(void* fs_data, const char* path, VFSStat* stat) {
  RamFS* fs = (RamFS*)fs_data;

  RamFSFile* file = find_file(fs, path);
  if (file == NULL) {
    return -1; // File doesn't exist
  }

  stat->size = file->size;
  stat->is_directory = file->is_directory;

  return 0;
}