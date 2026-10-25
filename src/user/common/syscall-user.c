#include "sys/types.h"
#include "sys/stat.h"
#include "unistd.h"

#include "syscall-common.h"


/*
* /----------------------\
* |x0   | syscall number |
* |x1   | return val ptr |
* |x2-x7| args           |
* \----------------------/
*/
long make_syscall(long number, long* ret, long a2, long a3, long a4, long a5, long a6, long a7) {
  register long x0 asm("x0") = number;
  register long x1 asm("x1") = (long)ret;
  register long x2 asm("x2") = a2;
  register long x3 asm("x3") = a3;
  register long x4 asm("x4") = a4;
  register long x5 asm("x5") = a5;
  register long x6 asm("x6") = a6;
  register long x7 asm("x7") = a7;

  asm volatile(
      "svc #0"
      : "+r"(x0), "+r"(x1), "+r"(x2), "+r"(x3),
        "+r"(x4), "+r"(x5), "+r"(x6), "+r"(x7)
      :
      : "memory", "cc");

  return *ret;
}


void _exit(int status) {
  make_syscall(SYS_EXIT, NULL, status, 0, 0, 0, 0, 0);
}

pid_t fork(void) {
  long ret = -1;
  make_syscall(SYS_FORK, &ret, 0, 0, 0, 0, 0, 0);
  return (pid_t)ret;
}

int execv(const char *path, char *const argv[]) {
  long ret = -1;
  make_syscall(SYS_EXECV, &ret, (long)path, (long)argv, 0, 0, 0, 0);
  return (int)ret;
}

pid_t getpid(void) {
  long ret = -1;
  make_syscall(SYS_GETPID, &ret, 0, 0, 0, 0, 0, 0);
  return (pid_t)ret;
}

unsigned int sleep(unsigned int seconds) {
  long ret = -1;
  make_syscall(SYS_SLEEP, &ret, seconds, 0, 0, 0, 0, 0);
  return (unsigned int)ret;
}

int open(const char *path, int flags, mode_t mode) {
  long ret = -1;
  make_syscall(SYS_OPEN, &ret, (long)path, flags, mode, 0, 0, 0);
  return (int)ret;
}

ssize_t write(int fd, const void *buf, size_t count) {
  long ret = -1;
  make_syscall(SYS_WRITE, &ret, fd, (long)buf, count, 0, 0, 0);
  return (ssize_t)ret;
}

ssize_t read(int fd, void *buf, size_t count) {
  long ret = -1;
  make_syscall(SYS_READ, &ret, fd, (long)buf, count, 0, 0, 0);
  return (ssize_t)ret;
}

int close(int fd) {
  long ret = -1;
  make_syscall(SYS_CLOSE, &ret, fd, 0, 0, 0, 0, 0);
  return (int)ret;
}

int mkdir(const char *path, mode_t mode) {
  long ret = -1;
  make_syscall(SYS_MKDIR, &ret, (long)path, mode, 0, 0, 0, 0);
  return (int)ret;
}

char *getcwd(char *buf, size_t size) {
  long ret = -1;
  make_syscall(SYS_GETCWD, &ret, (long)buf, size, 0, 0, 0, 0);
  return (char *)ret;
}

int chdir(const char *path) {
  long ret = -1;
  make_syscall(SYS_CHDIR, &ret, (long)path, 0, 0, 0, 0, 0);
  return (int)ret;
}

int stat(const char *path, struct stat *buf) {
  long ret = -1;
  make_syscall(SYS_STAT, &ret, (long)path, (long)buf, 0, 0, 0, 0);
  return (int)ret;
}
