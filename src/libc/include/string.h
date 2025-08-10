#ifndef STRING_H
#define STRING_H

#include <stddef.h>

/**
 * @brief Copies n bytes from memory area src to memory area dest.
 * 
 * @param dest Pointer to the destination array where the content is to be copied.
 * @param src Pointer to the source of data to be copied.
 * @param n Number of bytes to copy.
 * @return A pointer to dest.
 */
void* memcpy(void* dest, const void* src, size_t n);

/**
 * @brief Copies the C-string pointed to by src (including the null terminator) to dest.
 * 
 * @param dest Pointer to the destination array where the content is to be copied.
 * @param src Pointer to the null-terminated string to be copied.
 * @return A pointer to dest.
 */
char* strcpy(char* dest, const char* src);
void* memset(void* ptr, int value, size_t num);

#endif // STRING_H