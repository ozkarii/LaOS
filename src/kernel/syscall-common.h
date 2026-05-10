#ifndef SYSCALL_COMMON_H
#define SYSCALL_COMMON_H

// Process
#define SYS_EXIT    0
#define SYS_FORK    1
#define SYS_EXEC    2
#define SYS_GETPID  3

// Scheduling
#define SYS_SLEEP   10

// Files
#define SYS_OPEN    20
#define SYS_WRITE   21
#define SYS_READ    22
#define SYS_CLOSE   23

#endif // SYSCALL_COMMON_H