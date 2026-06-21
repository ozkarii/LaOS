#include <stddef.h>
#include "sys/types.h"

/* process */
void _exit(int status);
pid_t fork(void);
int execv(const char *pathname, char *const argv[]);
pid_t getpid(void);

/* scheduling / time */
unsigned int sleep(unsigned int seconds);

/* files */
int open(const char *path, int flags, int mode);
ssize_t write(int fd, const void *buf, size_t count);
ssize_t read(int fd, void *buf, size_t count);
int close(int fd);