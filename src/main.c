/**
 * @file main.c
 * @brief Entry point to the Laudes operating system.
 */

#include "io.h"
#include "cpu.h"
#include "stdio.h"
#include "stdint.h"

#define HELLO "Hello Laudes"

int c_entry() {
  k_printf("%s 0x%lx\r\n", HELLO, 0xDEADBEEFF00DBAD);
  while(1) {};
} 
