#include <stdarg.h>
#include <stdarg.h>
#include "pl011.h"
#include "stdio.h"
#include "io.h"
#include "serial-buffer.h"
#include "spinlock.h"

Spinlock k_puts_lock = SPINLOCK_INIT_IRQ_SAFE;
bool released = false;

void k_log_release(void) {
  released = true;
}

void k_putchar(const char c) {
  if (released) {
        
  }
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
  return serial_buffer_get();
}

void k_printf(const char* format, ...) {
  char buffer[256];
  va_list args;
  va_start(args, format);
  vsnprintf(buffer, sizeof(buffer), format, args);
  va_end(args);

  k_puts(buffer);
}