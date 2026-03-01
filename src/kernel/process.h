#ifndef PROCESS_H
#define PROCESS_H

#include <stdint.h>

#define TEXT_SECTION_VA 0x200000
#define DATA_SECTION_VA 0x400000
#define STACK_TOP_VA    0x600000

#define MAX_PROCESSES 64
#define MAX_OPEN_FDS 16

typedef int32_t pid_t;
typedef int32_t fd_t;

pid_t process_create_init_process(void);
void process_destroy(pid_t pid);
pid_t process_clone(pid_t parent_pid);

#endif // PROCESS_H