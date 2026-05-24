#include "sys/types.h"
#include "unistd.h"

#include "syscall-common.h"


/*
* /----------------------\
* |x0   | syscall number |
* |x1   | return value
* |x2   | number of args |
* |x3-x9| args           |
* \----------------------/
*/
long make_syscall(long number, long* ret, ...) {
  (void)number;

  asm volatile(
      "svc #0"
      :
      :
      : "x1", "x2", "x3", "x4", "x5", "x6", "x7", "memory"
  );

  return *ret;
}


void _exit(int status) {
  long ret;
  make_syscall(SYS_EXIT, &ret, status);
}

pid_t fork(void) {
  long ret;
  make_syscall(SYS_FORK, &ret);
  return (pid_t)ret;
}

int execve(const char *path, char *const argv[], char *const envp[]) {
  long ret;
  make_syscall(SYS_EXEC, &ret, path, argv, envp);
  return (int)ret;
}

pid_t getpid(void) {
  long ret;
  make_syscall(SYS_GETPID, &ret);
  return (pid_t)ret;
}

unsigned int sleep(unsigned int seconds) {
  long ret;
  make_syscall(SYS_SLEEP, &ret, seconds);
  return (unsigned int)ret;
}

int open(const char *path, int flags, int mode) {
  long ret;
  make_syscall(SYS_OPEN, &ret, path, flags, mode);
  return (int)ret;
}

ssize_t write(int fd, const void *buf, size_t count) {
  long ret;
  make_syscall(SYS_WRITE, &ret, fd, buf, count);
  return (ssize_t)ret;
}

ssize_t read(int fd, void *buf, size_t count) {
  long ret;
  make_syscall(SYS_READ, &ret, fd, buf, count);
  return (ssize_t)ret;
}

int close(int fd) {
  long ret;
  make_syscall(SYS_CLOSE, &ret, fd);
  return (int)ret;
}
