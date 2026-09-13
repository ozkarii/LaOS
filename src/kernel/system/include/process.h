#ifndef PROCESS_H
#define PROCESS_H

#include <stdint.h>

#include "sys/types.h"

#define STACK_TOP_VA    0x600000

typedef int32_t pid_t;

pid_t process_create_init_process(void);
int process_destroy(pid_t pid);

// Fork process. The child is blocked until process_release_cloned_task is called.
pid_t process_clone(pid_t parent_pid);

// Makes a cloned child schedulable. Call only once and after process_clone.
int process_release_cloned_task(pid_t pid);

uint64_t* process_get_l2_table(pid_t pid);
int process_load_l2_table(pid_t pid);
int process_unload_l2_table(pid_t pid);

int process_open_file(pid_t pid, const char* path, int flags, mode_t mode);
ssize_t process_write_file(pid_t pid, int fd, const void* buffer, size_t size);
ssize_t process_read_file(pid_t pid, int fd, void* buffer, size_t size);
int process_close_file(pid_t pid, int fd);
int process_replace_executable(pid_t pid, const char* path);

#endif // PROCESS_H