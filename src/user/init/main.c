#include <stdarg.h>

#include "unistd.h"
#include "fcntl.h"

#include "stdio.h"

#define stdout 1

const char message[] = "Hello world!\n";

int printf(const char* format, ...) {
  char buffer[256];
  va_list args;
  va_start(args, format);
  int len = vsnprintf(buffer, sizeof(buffer), format, args);
  va_end(args);

  if (len > 0) {
    len = write(1, buffer, (size_t)len);
  }

  return len;
}

int main() {
  write(stdout, message, sizeof(message) - 1);

  pid_t pid = getpid();    

  printf("Hello from init process! PID: %d\n", pid);
  
  const char filename[] = "/testfile";
  int fd1 = open(filename , O_CREAT | O_RDWR, 0);
  printf("Created file %s with fd: %d\n", filename, fd1);

  int ret = write(fd1, message, sizeof(message));
  printf("Wrote '%s' (%d bytes) to file %s\n", message, ret, filename);

  ret = close(fd1);
  printf("Closed file %s, ret=%d\n", filename, ret);

  int fd2 = open(filename, O_RDONLY, 0);
  printf("Re-opened file %s as read-only with fd: %d\n", filename, fd2);

  char buffer[64];
  ssize_t bytes_read = read(fd2, buffer, sizeof(buffer) - 1);
  if (bytes_read < 0) {
    printf("Failed to read from file %s, ret=%d\n", filename, (int)bytes_read);
  } else {
    buffer[bytes_read] = '\0';
    printf("Read %lu bytes from file %s: '%s'\n", bytes_read, filename, buffer);
  }

  ret = close(fd2);
  printf("Closed file %s, ret=%d\n", filename, ret);

  /* TODO: fix context switch to child process, probably x30 register is wrong
  pid_t child_pid = fork();
  if (child_pid < 0) {
    printf("Fork failed\n");
  } else if (child_pid == 0) {
    printf("Hello from child process! PID: %d\n", getpid());
  } else {
    printf("Hello from parent process! PID: %d, child PID: %d\n", getpid(), child_pid);
  }
  */

  return 0;
}
