#include <stdarg.h>

#include "string.h"
#include "stdio.h"

#include "libc-impl.h"

void* memcpy(void* dest, const void* src, size_t n) {
	return memcpy_impl(dest, src, n);
}

void* memset(void* ptr, int value, size_t num) {
	return memset_impl(ptr, value, num);
}

char* strcpy(char* dest, const char* src) {
	return strcpy_impl(dest, src);
}

int strcmp(const char* str1, const char* str2) {
	return strcmp_impl(str1, str2);
}

size_t strlen(const char* str) {
	return strlen_impl(str);
}

int snprintf(char* buf, size_t size, const char* format, ...) {
    if (buf == NULL || format == NULL) {
        return -1;
    }
    va_list args;
    va_start(args, format);
    int result = vsnprintf(buf, size, format, args);
    va_end(args);
    return result;
}

int vsnprintf(char* buf, size_t size, const char* format, va_list args) {
	return vsnprintf_impl(buf, size, format, args);
}
