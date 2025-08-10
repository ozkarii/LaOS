/**
 * @file main.c
 * @brief Entry point to the Laudes operating system.
 */

#include <stdint.h>
#include "stdio.h"
#include "string.h"

#include "io.h"
#include "console.h"

int c_entry() {
  console_loop("#");
  return 0;
} 
