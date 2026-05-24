#include "unistd.h"
#include "fcntl.h"

#define stdout 1

const char message[] = "Hello world!\n";

int main() {
  pid_t pid = getpid();
  (void)pid;

  write(1, message, pid + 10);

  open("/testfile", O_CREAT, 0);

  while (1) {
    // Infinite loop to prevent process from exiting
  }

  return 10;
}
