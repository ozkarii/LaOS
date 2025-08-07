#include "platform.h"
#include "pl011.h"

static inline void pl011_write_reg32(unsigned long offset, unsigned int val) {
  *(volatile unsigned int*)(PL011_BASE + offset) = val;
}

static inline unsigned int pl011_read_reg32(unsigned long offset) {
  return *(volatile unsigned int*)(PL011_BASE + offset);
}

int pl011_busy(void) {
  return (pl011_read_reg32(UARTFR) & UARTFR_BUSY);
}

void pl011_putc(const char c) {
  while (pl011_busy()) {}
  pl011_write_reg32(UARTDR, c);
}
