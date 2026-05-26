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
  
  int fd1 = open("/testfile", O_CREAT | O_RDWR, 0);
  printf("Opened file with fd: %d\n", fd1);

  int ret = write(fd1, message, sizeof(message));
  printf("Wrote %d bytes to file\n", ret);

  ret = close(fd1);
  printf("Closed file, ret=%d\n", ret);

  int fd2 = open("/testfile", O_RDONLY, 0);
  printf("Re-opened file with fd: %d\n", fd2);

  char buffer[64];
  ssize_t bytes_read = read(fd2, buffer, sizeof(buffer) - 1);
  if (bytes_read < 0) {
    printf("Failed to read from file, ret=%d\n", (int)bytes_read);
  } else {
    buffer[bytes_read] = '\0';
    printf("Read from file: %s\n", buffer);
  }

  return 10;
}
