#ifndef SYSCALL_KERNEL_H
#define SYSCALL_KERNEL_H

#include <stdint.h>
#include <sys/types.h>

#include "sem.h"
#include "syscall-common.h"

typedef struct SyscallContext {
  pid_t pid;
  task_id_t task_id;
  long args[MAX_SYSCALL_PARAMS];
  long* ret;
} SyscallContext;


/**
 * Call only from syscall sync exception context.
 * Creates a kernel task to handle syscall with given args
 * Sets the user process that made the syscall as blocked,
 * but it must be yielded manually right after eret.
 * Handler task will wake it up once the syscall is handled.
 * Returns 0
 * Syscall return value is 
 */
int syscall_handler(long number, long* ret, ...);

#endif // SYSCALL_KERNEL_H