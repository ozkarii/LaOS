#include "string.h"

#include "ramdisk.h"

bool ramdisk_is_valid(const void* image) {
  const RamdiskHeader* header = (const RamdiskHeader*)image;
  return strcmp(header->magic, RAMDISK_MAGIC) == 0;
}

uint32_t ramdisk_get_file_count(const void* image) {
  return ((const RamdiskHeader*)image)->file_count;
}

const RamdiskEntry* ramdisk_get_entry(const void* image, uint32_t index) {
  const RamdiskHeader* header = (const RamdiskHeader*)image;

  if (index >= header->file_count) {
    return NULL;
  }

  const uint8_t* table = (const uint8_t*)image + header->table_offset;
  return &((const RamdiskEntry*)table)[index];
}

const uint8_t* ramdisk_get_data(const void* image, const RamdiskEntry* entry) {
  return (const uint8_t*)image + entry->offset;
}
