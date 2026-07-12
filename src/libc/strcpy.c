#include "string.h"

char* strcpy_impl(char* dest, const char* src) {
  char* ptr = dest;
  while ((*ptr++ = *src++));
  return dest;
}
