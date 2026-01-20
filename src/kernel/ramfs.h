#ifndef RAMFS_H
#define RAMFS_H

#include <stdint.h>
#include <stddef.h>
#include "vfs.h"


VFSInterface* ramfs_get_vfs_interface(void);
void* ramfs_init(void);

#endif // RAMFS_H