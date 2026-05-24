#include "unistd.h"

#define stdout 1

const char message[] = "Hello world\n";

int main() {
  pid_t pid = getpid();
  (void)pid;

  write(1, message, pid);

  while (1) {
    // Infinite loop to prevent process from exiting
  }

  return 10;
}
