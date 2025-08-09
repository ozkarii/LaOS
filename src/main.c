/**
 * @file main.c
 * @brief Entry point to the Laudes operating system.
 */

#include "stdio.h"
#include "stdint.h"

#include "io.h"
#include "console.h"

int c_entry() {
  console_loop("laulau #");
  return 0;
} 
