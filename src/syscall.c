#include "pl011.h"

void putc(const char c) {
  pl011_putc(c);
}

void puts(const char* s) {
  char* tmp = (char*)s;
  while(*tmp) {
    putc(*tmp);
    tmp++;
  }
}
