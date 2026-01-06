#include <stdarg.h>
#include <stdarg.h>
#include "pl011.h"
#include "stdio.h"
#include "io.h"
#include "io-buffer.h"
#include "spinlock.h"

static Spinlock k_puts_lock = {0};

void k_putchar(const char c) {
  pl011_putc(c);
}

void k_puts(const char* s) {
  char* tmp = (char*)s;
  spinlock_acquire(&k_puts_lock);
  while(*tmp) {
    k_putchar(*tmp);
    tmp++;
  }
  spinlock_release(&k_puts_lock);
}

char k_getchar(void) {
  return serial_buffer_getc();
}

char* k_gets(char* s, int max_len) {
  char* tmp = s;
  int count = 0;

  while (count < max_len - 1) {
    char c = k_getchar();
    if (c == '\n' || c == '\r' || EOF) {
      break;
    }
    *tmp++ = c;
    count++;
  }
  
  *tmp = '\0'; // Null-terminate the string
  return s;
}

void k_printf(const char* format, ...) {
  char buffer[256];
  va_list args;
  va_start(args, format);
  vsnprintf(buffer, sizeof(buffer), format, args);
  va_end(args);

  k_puts(buffer);
}