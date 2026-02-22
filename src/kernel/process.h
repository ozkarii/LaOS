#ifndef PROCESS_H
#define PROCESS_H

#include <stdint.h>

#define MAX_PROCESSES 64
#define MAX_OPEN_FDS 16

typedef int32_t pid_t;
typedef int32_t fd_t;

pid_t process_create_init_process(void);
void process_destroy(pid_t pid);
pid_t process_clone(pid_t parent_pid);

#endif // PROCESS_H