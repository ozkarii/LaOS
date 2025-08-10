#ifndef STRING_H
#define STRING_H

#include <stddef.h>

void* memcpy(void* dest, const void* src, size_t n);
char* strcpy(char* dest, const char* src);
void* memset(void* ptr, int value, size_t num);

#endif // STRING_H