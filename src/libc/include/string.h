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

void* memset(void* ptr, int value, size_t num);

/**
 * @brief Copies the C-string pointed to by src (including the null terminator) to dest.
 * 
 * @param dest Pointer to the destination array where the content is to be copied.
 * @param src Pointer to the null-terminated string to be copied.
 * @return A pointer to dest.
 */
char* strcpy(char* dest, const char* src);


/**
 * @brief Compares two null-terminated C strings.
 * 
 * The comparison is performed character by character using unsigned char values.
 * It stops at the first differing character or the null terminator.
 * 
 * @param str1 Pointer to the first null-terminated string.
 * @param str2 Pointer to the second null-terminated string.
 * @return An integer less than, equal to, or greater than zero:
 *         - < 0 if str1 is less than str2
 *         - = 0 if str1 is equal to str2
 *         - > 0 if str1 is greater than str2
 */
int strcmp ( const char * str1, const char * str2 );

/**
 * @brief Calculates the length of a given string.
 * 
 * It doesn't count the null character '\0'.
 * 
 * @param str Pointer to the null-terminated string, whose lenght we need to find.
 * @return The integral length of the string passed
 */
size_t strlen(const char * str);

#endif // STRING_H