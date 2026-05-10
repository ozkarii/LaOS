#include "unistd.h"

#define stdout 1

const char message[] = "Hello userland!\n";

int main() {
  write(1, message, sizeof(message) - 1);
  _exit(0xdead);
}
