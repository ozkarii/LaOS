#ifndef PROCESS_H
#define PROCESS_H

#include <stdint.h>

#include "sys/types.h"

#define STACK_TOP_VA    0x600000

#define MAX_PROCESSES 64
#define MAX_OPEN_FDS 16
#define MAX_VIRTUAL_MEMORY_MAPPINGS 16

typedef int32_t pid_t;

pid_t process_create_init_process(void);
int process_destroy(pid_t pid);
pid_t process_clone(pid_t parent_pid);

int process_load_l2_table(pid_t pid);
int process_unload_l2_table(pid_t pid);

int process_open_file(pid_t pid, const char* path, int flags, int mode);
ssize_t process_write_file(pid_t pid, int fd, const void* buffer, size_t size);
ssize_t process_read_file(pid_t pid, int fd, void* buffer, size_t size);
int process_close_file(pid_t pid, int fd);

#endif // PROCESS_H