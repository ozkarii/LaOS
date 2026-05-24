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
 * Sets the user process that made the syscall as blocked
 * and yields right after returning from exception. This
 * assumes that the timer IRQ will fire right after returning to EL0.
 * Handler task will wake up the user process once the syscall is handled.
 * Syscall return value is written to the `ret` pointer in user process address space.
 * Returns 0
 */
int syscall_handler(long number, long* ret, ...);

#endif // SYSCALL_KERNEL_H