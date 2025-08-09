#include "platform.h"
#include "pl011.h"

static inline void pl011_write_reg32(uint64_t offset, uint32_t val) {
  *(volatile uint32_t*)(PL011_BASE + offset) = val;
}

static inline uint32_t pl011_read_reg32(uint64_t offset) {
  return *(volatile uint32_t*)(PL011_BASE + offset);
}

int pl011_busy(void) {
  return (pl011_read_reg32(UARTFR) & UARTFR_BUSY);
}

int pl011_rx_full(void) {
  int res = (int)(pl011_read_reg32(UARTFR) & UARTFR_RXFF);
  return res;
}

void pl011_putc(const char c) {
  while (pl011_busy()) {}
  pl011_write_reg32(UARTDR, c);
}

char pl011_getc(void) {
  while (!pl011_rx_full()) {}
  return (char)(pl011_read_reg32(UARTDR) & 0xFF);
}
