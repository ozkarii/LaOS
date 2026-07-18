#include <stdarg.h>

#include "sys/types.h"
#include "unistd.h"

#include "syscall-common.h"


/*
* /----------------------\
* |x0   | syscall number |
* |x1   | return val ptr |
* |x2-x7| args           |
* \----------------------/
*/
long make_syscall(long number, long* ret, ...) {
  register long x0 asm("x0") = number;
  register long x1 asm("x1") = (long)ret;
  register long x2 asm("x2");
  register long x3 asm("x3");
  register long x4 asm("x4");
  register long x5 asm("x5");
  register long x6 asm("x6");
  register long x7 asm("x7");

  va_list ap;
  va_start(ap, ret);

  x2 = va_arg(ap, long);
  x3 = va_arg(ap, long);
  x4 = va_arg(ap, long);
  x5 = va_arg(ap, long);
  x6 = va_arg(ap, long);
  x7 = va_arg(ap, long);

  va_end(ap);

  asm volatile(
      "svc #0"
      : "+r"(x0), "+r"(x1), "+r"(x2), "+r"(x3),
        "+r"(x4), "+r"(x5), "+r"(x6), "+r"(x7)
      :
      : "memory", "cc");

  return *ret;
}


void _exit(int status) {
  make_syscall(SYS_EXIT, NULL, status);
}

pid_t fork(void) {
  long ret = -1;
  make_syscall(SYS_FORK, &ret);
  return (pid_t)ret;
}

int execv(const char *path, char *const argv[]) {
  long ret = -1;
  make_syscall(SYS_EXECV, &ret, path, argv);
  return (int)ret;
}

pid_t getpid(void) {
  long ret = -1;
  make_syscall(SYS_GETPID, &ret);
  return (pid_t)ret;
}

unsigned int sleep(unsigned int seconds) {
  long ret = -1;
  make_syscall(SYS_SLEEP, &ret, seconds);
  return (unsigned int)ret;
}

int open(const char *path, int flags, int mode) {
  long ret = -1;
  make_syscall(SYS_OPEN, &ret, path, flags, mode);
  return (int)ret;
}

ssize_t write(int fd, const void *buf, size_t count) {
  long ret = -1;
  make_syscall(SYS_WRITE, &ret, fd, buf, count);
  return (ssize_t)ret;
}

ssize_t read(int fd, void *buf, size_t count) {
  long ret = -1;
  make_syscall(SYS_READ, &ret, fd, buf, count);
  return (ssize_t)ret;
}

int close(int fd) {
  long ret = -1;
  make_syscall(SYS_CLOSE, &ret, fd);
  return (int)ret;
}
