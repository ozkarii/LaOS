#ifndef LIBC_IMPL_H
#define LIBC_IMPL_H

void* memcpy_impl(void* dest, const void* src, size_t n);
void* memset_impl(void* ptr, int value, size_t num);
char* strcpy_impl(char* dest, const char* src);
int strcmp_impl(const char* str1, const char* str2);
size_t strlen_impl(const char* str);
int snprintf_impl(char* buf, size_t size, const char* format, ...);
int vsnprintf_impl(char* buf, size_t size, const char* format, va_list args);

#endif // LIBC_IMPL_H