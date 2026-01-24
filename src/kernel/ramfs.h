#ifndef RAMFS_H
#define RAMFS_H

#include <stdint.h>
#include <stddef.h>
#include "vfs.h"


VFSInterface* ramfs_get_vfs_interface(void);
size_t ramfs_get_size(void);
void* ramfs_init(void* dest, size_t max_size);

#endif // RAMFS_H