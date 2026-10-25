#ifndef SYSCALL_COMMON_H
#define SYSCALL_COMMON_H

#define SYS_EXIT    0
#define SYS_FORK    1
#define SYS_EXECV   2
#define SYS_GETPID  3
#define SYS_SLEEP   4

// Files
#define SYS_OPEN    10
#define SYS_WRITE   11
#define SYS_READ    12
#define SYS_CLOSE   13
#define SYS_MKDIR   14
#define SYS_STAT    15
#define SYS_GETCWD  16
#define SYS_CHDIR   17

#define MAX_SYSCALL_PARAMS 6

#endif // SYSCALL_COMMON_H