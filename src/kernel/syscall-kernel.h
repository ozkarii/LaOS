#ifndef SYSCALL_KERNEL_H
#define SYSCALL_KERNEL_H

#include <stdint.h>
#include <sys/types.h>
#include "sem.h"

typedef struct SyscallContext {
  pid_t pid;
  long args[8];
  long ret;
} SyscallContext;

long syscall_handler(long number, int num_args, ...);

#endif // SYSCALL_KERNEL_H