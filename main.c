/**
 * @brief PL011 UART For raspi4b
 */

#define PL011_BASE 0xfe201000
// #define PL011_BASE 0x3f201000

#define UARTDR 0x0

#define UARTFR 0x18
#define UARTFR_BUSY (1 << 3u)
#define UARTFR_TXFF (1 << 5u)

#define UARTCR 0x30
#define UARTCR_TXE (1 << 8u)

static inline void pl011_write_reg8(unsigned long offset, unsigned char val) {
  *(volatile unsigned char*)(PL011_BASE + offset) = val;
}

static inline void pl011_write_reg32(unsigned long offset, unsigned int val) {
  *(volatile unsigned int*)(PL011_BASE + offset) = val;
}

static inline unsigned int pl011_read_reg32(unsigned long offset) {
  return *(volatile unsigned int*)(PL011_BASE + offset);
}

static inline unsigned int get_mpidr(void) {
    unsigned int mpidr;
    asm volatile ("mrs %0, mpidr_el1" : "=r"(mpidr));
    return mpidr;
}

void putc(const char c) {
  while (pl011_read_reg32(UARTFR) & (UARTFR_BUSY));
  pl011_write_reg8(UARTDR, (unsigned char)c);
}

void puts(const char* s) {
  char* tmp = (char*)s;
  while(*tmp) {
    putc(*tmp);
    tmp++;
  }
}

int c_entry() {
  const char* str = "Hello Laudes\r\n";
  if ((get_mpidr() & 0xFF) == 0) {
    puts(str);
  }
  while(1);
}
  