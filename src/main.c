/**
 * @file main.c
 * @brief Main entry point to the Laudes operating system.
 */

#include "syscall.h"

static inline unsigned int get_mpidr(void) {
    unsigned int mpidr;
    asm volatile ("mrs %0, mpidr_el1" : "=r"(mpidr));
    return mpidr;
}

int c_entry() {
  const char* str = "Hello Laudes\r\n";
  if ((get_mpidr() & 0xFF) == 0) {
    puts(str);
    puts("This is the first core.\r\n");
  }
  while(1) {};
}
  
