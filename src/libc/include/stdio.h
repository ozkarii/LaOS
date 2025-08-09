#ifndef STDIO_H
#define STDIO_H

#include <stddef.h>
#include <stdarg.h>

#define EOF (-1)

int vsnprintf(char* buf, size_t size, const char* format, va_list args);
int snprintf(char* buf, size_t size, const char* format, ...)
    __attribute__((format(printf, 3, 4)));


#endif // STDIO_H