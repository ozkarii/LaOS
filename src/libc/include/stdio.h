#ifndef STDIO_H
#define STDIO_H

#include <stddef.h>
#include <stdarg.h>

#define EOF (-1)

int vsnprintf(char* buf, size_t size, const char* format, va_list args);

/**
 * @brief Writes formatted data to a string.
 *
 * Formats and stores a series of characters and values in the buffer `buf` according to the format string `format`.
 * At most `size` bytes are written, including the terminating null byte.
 * If the output is truncated due to the size limit, the return value is the number of characters (excluding the null byte) that would have been written.
 *
 * @param buf Pointer to the buffer where the formatted string will be stored.
 * @param size Maximum number of bytes to be written to the buffer, including the null terminator.
 * @param format Format string specifying how subsequent arguments are converted for output.
 * @param ... Additional arguments to be formatted according to the format string.
 * @return The number of characters that would have been written if enough space had been available, not counting the terminating null byte.
 */
int snprintf(char* buf, size_t size, const char* format, ...)
    __attribute__((format(printf, 3, 4)));


#endif // STDIO_H