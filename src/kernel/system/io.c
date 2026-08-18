#include <stdarg.h>
#include <stdarg.h>
#include "pl011.h"
#include "stdio.h"
#include "io.h"
#include "serial-buffer.h"
#include "spinlock.h"

static Spinlock io_lock = {0};

static inline void k_putchar_no_lock(const char c) {
  pl011_putc(c);
}

void k_putchar(const char c) {
  spinlock_acquire(&io_lock);
  k_putchar_no_lock(c);
  spinlock_release(&io_lock);
}

void k_puts(const char* s) {
  char* tmp = (char*)s;
  spinlock_acquire(&io_lock);
  while(*tmp) {
    k_putchar_no_lock(*tmp);
    tmp++;
  }
  spinlock_release(&io_lock);
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