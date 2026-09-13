#ifndef RAMDISK_H
#define RAMDISK_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "path.h"

#define RAMDISK_MAGIC "LRD1"

typedef struct __attribute__((packed)) RamdiskHeader {
  char magic[5]; // "LRD1", NUL-terminated
  uint32_t file_count;
  uint32_t table_offset;
  uint32_t data_offset;
} RamdiskHeader;

typedef struct __attribute__((packed)) RamdiskEntry {
  char path[PATH_NAME_MAX];
  uint32_t offset;
  uint32_t size;
} RamdiskEntry;

// Returns true if `image` starts with a valid magic
bool ramdisk_is_valid(const void* image);

// Number of files packed in the ramdisk image
uint32_t ramdisk_get_file_count(const void* image);

// Returns the entry at `index`, or NULL if out of range
const RamdiskEntry* ramdisk_get_entry(const void* image, uint32_t index);

// Returns a pointer to the file data described by `entry`
const uint8_t* ramdisk_get_data(const void* image, const RamdiskEntry* entry);

#endif // RAMDISK_H
