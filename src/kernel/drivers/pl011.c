#include "platform.h"
#include "pl011.h"

// Use just UART0 for now
#define PL011_BASE UART0_BASE

static inline void pl011_write_reg32(uint64_t offset, uint32_t val) {
  *(volatile uint32_t*)(PL011_BASE + offset) = val;
}

static inline uint32_t pl011_read_reg32(uint64_t offset) {
  return *(volatile uint32_t*)(PL011_BASE + offset);
}

void pl011_enable(void) {
  pl011_write_reg32(UARTCR, 
    pl011_read_reg32(UARTCR) | UARTCR_UARTEN | UARTCR_RXE | UARTCR_TXE);
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

void pl011_set_rx_irq(bool enable) {
  uint32_t read = pl011_read_reg32(UARTIMSC);
  if (enable) {
    pl011_write_reg32(UARTIMSC, read | UARTIMSC_RXIM);
  }
  else {
    pl011_write_reg32(UARTIMSC, read & ~UARTIMSC_RXIM);
  }
}

uint32_t pl011_get_raw_irq_status(void) {
  return pl011_read_reg32(UARTRIS);
}

uint32_t pl011_get_masked_irq_status(void) {
  return pl011_read_reg32(UARTMIS);
}

void pl011_print_info(void (*printf_func)(const char* format, ...)) {
  printf_func("PL011 UART at 0x%x:\r\n", PL011_BASE);
  
  // Data Register
  uint32_t dr = pl011_read_reg32(UARTDR);
  printf_func("  UARTDR: 0x%x (Data: 0x%x)\r\n", dr, dr & 0xFF);
  
  // Flag Register
  uint32_t fr = pl011_read_reg32(UARTFR);
  printf_func("  UARTFR: 0x%x ", fr);
  printf_func("(BUSY:%d TXFF:%d RXFF:%d)\r\n",
              (fr & UARTFR_BUSY) ? 1 : 0,
              (fr & UARTFR_TXFF) ? 1 : 0,
              (fr & UARTFR_RXFF) ? 1 : 0);
  
  // Line Control Register
  uint32_t lcr_h = pl011_read_reg32(UARTLCR_H);
  printf_func("  UARTLCR_H: 0x%x (FEN:%d)\r\n", 
              lcr_h, (lcr_h & UARTLCR_H_FEN) ? 1 : 0);
  
  // Control Register
  uint32_t cr = pl011_read_reg32(UARTCR);
  printf_func("  UARTCR: 0x%x (UARTEN:%d TXE:%d RXE:%d)\r\n",
              cr,
              (cr & UARTCR_UARTEN) ? 1 : 0,
              (cr & UARTCR_TXE) ? 1 : 0,
              (cr & UARTCR_RXE) ? 1 : 0);
  
  // Interrupt Mask Set/Clear Register
  uint32_t imsc = pl011_read_reg32(UARTIMSC);
  printf_func("  UARTIMSC: 0x%x (RXIM:%d) (TXIM:%d)\r\n",
                imsc, 
                (imsc & UARTIMSC_RXIM) ? 1 : 0,
                (imsc & UARTIMSC_TXIM) ? 1 : 0);
  
  // Raw Interrupt Status Register
  uint32_t ris = pl011_read_reg32(UARTRIS);
  printf_func("  UARTRIS: 0x%x (RXRIS:%d TXRIS:%d)\r\n",
              ris,
              (ris & UARTRIS_RXRIS) ? 1 : 0,
              (ris & UARTRIS_TXRIS) ? 1 : 0);
  
  // Masked Interrupt Status Register
  uint32_t mis = pl011_read_reg32(UARTMIS);
  printf_func("  UARTMIS: 0x%x\r\n", mis);
}
