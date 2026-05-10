#include "sys/types.h"
#include "unistd.h"

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



/*
* /----------------------\
* |x0   | syscall number |
* |x1   | number of args |
* |x2-x8| args           |
* \----------------------/
*/
long make_syscall(long number, int num_args, ...) {
    register long x0 asm("x0") = number;

    asm volatile(
        "svc #0"
        : "+r"(x0)
        :
        : "x1", "x2", "x3", "x4", "x5", "x6", "x7", "x8", "memory"
    );

    return x0;
}


void _exit(int status) {
  make_syscall(SYS_EXIT, 1, status);
}

pid_t fork(void) {
  return (pid_t)make_syscall(SYS_FORK, 0);
}

int execve(const char *path, char *const argv[], char *const envp[]) {
  return (int)make_syscall(SYS_EXEC, 3, path, argv, envp);
}

pid_t getpid(void) {
  return (pid_t)make_syscall(SYS_GETPID, 0);
}

unsigned int sleep(unsigned int seconds) {
  return (unsigned int)make_syscall(SYS_SLEEP, 1, seconds);
}

int open(const char *path, int flags, int mode) {
  return (int)make_syscall(SYS_OPEN, 3, path, flags, mode);
}

ssize_t write(int fd, const void *buf, size_t count) {
  return (ssize_t)make_syscall(SYS_WRITE, 3, fd, buf, count);
}

ssize_t read(int fd, void *buf, size_t count) {
  return (ssize_t)make_syscall(SYS_READ, 3, fd, buf, count);
}

int close(int fd) {
  return (int)make_syscall(SYS_CLOSE, 1, fd);
}
