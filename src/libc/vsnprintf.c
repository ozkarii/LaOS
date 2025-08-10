#include <stdarg.h>
#include "include/stdio.h"

static int int_to_str(int val, char* buf, int base) {
    if (val == 0) {
        buf[0] = '0';
        buf[1] = '\0';
        return 1;
    }
    
    char temp[16];
    int i = 0;
    int is_negative = 0;
    
    // Handle negative numbers for base 10
    if (val < 0 && base == 10) {
        is_negative = 1;
        val = -val;
    }
    
    // Convert to string in reverse order
    while (val > 0) {
        int digit = val % base;
        if (digit < 10) {
            temp[i++] = '0' + digit;
        } else {
            temp[i++] = 'a' + (digit - 10);
        }
        val /= base;
    }
    
    int len = 0;
    
    // Add negative sign if needed
    if (is_negative) {
        buf[len++] = '-';
    }
    
    // Copy reversed string to output buffer
    while (i > 0) {
        buf[len++] = temp[--i];
    }
    buf[len] = '\0';
    
    return len;
}

static int uint_to_str(unsigned int val, char* buf, int base) {
    if (val == 0) {
        buf[0] = '0';
        buf[1] = '\0';
        return 1;
    }
    
    char temp[16];
    int i = 0;
    
    // Convert to string in reverse order
    while (val > 0) {
        unsigned int digit = val % base;
        if (digit < 10) {
            temp[i++] = '0' + digit;
        } else {
            temp[i++] = 'a' + (digit - 10);
        }
        val /= base;
    }
    
    int len = 0;
    
    // Copy reversed string to output buffer
    while (i > 0) {
        buf[len++] = temp[--i];
    }
    buf[len] = '\0';
    
    return len;
}

static int long_to_str(long val, char* buf, int base) {
    if (val == 0) {
        buf[0] = '0';
        buf[1] = '\0';
        return 1;
    }
    
    char temp[32];  // Increased size for long values
    int i = 0;
    int is_negative = 0;
    
    // Handle negative numbers for base 10
    if (val < 0 && base == 10) {
        is_negative = 1;
        val = -val;
    }
    
    // Convert to string in reverse order
    while (val > 0) {
        long digit = val % base;
        if (digit < 10) {
            temp[i++] = '0' + digit;
        } else {
            temp[i++] = 'a' + (digit - 10);
        }
        val /= base;
    }
    
    int len = 0;
    
    // Add negative sign if needed
    if (is_negative) {
        buf[len++] = '-';
    }
    
    // Copy reversed string to output buffer
    while (i > 0) {
        buf[len++] = temp[--i];
    }
    buf[len] = '\0';
    
    return len;
}

static int ulong_to_str(unsigned long val, char* buf, int base) {
    if (val == 0) {
        buf[0] = '0';
        buf[1] = '\0';
        return 1;
    }
    
    char temp[32];  // Increased size for long values
    int i = 0;
    
    // Convert to string in reverse order
    while (val > 0) {
        unsigned long digit = val % base;
        if (digit < 10) {
            temp[i++] = '0' + digit;
        } else {
            temp[i++] = 'a' + (digit - 10);
        }
        val /= base;
    }
    
    int len = 0;
    
    // Copy reversed string to output buffer
    while (i > 0) {
        buf[len++] = temp[--i];
    }
    buf[len] = '\0';
    
    return len;
}

int vsnprintf(char* buf, size_t size, const char* format, va_list args) {
    if (!buf || size == 0) return 0;

    char* p = buf;
    const char* f = format;
    size_t written = 0;

    while (*f && written < size - 1) {
        if (*f == '%') {
            f++;
            if (*f == '\0') {
                // Format string ends with '%', just add it literally
                if (written < size - 1) {
                    *p++ = '%';
                    written++;
                }
                break;
            }
            
            // Check for 'l' modifier
            int is_long = 0;
            if (*f == 'l') {
                is_long = 1;
                f++;
            }
            
            if (*f == 'd') {
                char numbuf[32];
                int len;
                if (is_long) {
                    long val = va_arg(args, long);
                    len = long_to_str(val, numbuf, 10);
                } else {
                    int val = va_arg(args, int);
                    len = int_to_str(val, numbuf, 10);
                }
                for (int i = 0; i < len && written < size - 1; i++) {
                    *p++ = numbuf[i];
                    written++;
                }
                f++;
            } else if (*f == 'x') {
                char numbuf[32];
                int len;
                if (is_long) {
                    unsigned long val = va_arg(args, unsigned long);
                    len = ulong_to_str(val, numbuf, 16);
                } else {
                    unsigned int val = va_arg(args, unsigned int);
                    len = uint_to_str(val, numbuf, 16);
                }
                for (int i = 0; i < len && written < size - 1; i++) {
                    *p++ = numbuf[i];
                    written++;
                }
                f++;
            } else if (*f == 'u') {
                // Add unsigned decimal support
                char numbuf[32];
                int len;
                if (is_long) {
                    unsigned long val = va_arg(args, unsigned long);
                    len = ulong_to_str(val, numbuf, 10);
                } else {
                    unsigned int val = va_arg(args, unsigned int);
                    len = uint_to_str(val, numbuf, 10);
                }
                for (int i = 0; i < len && written < size - 1; i++) {
                    *p++ = numbuf[i];
                    written++;
                }
                f++;
            } else if (*f == 's') {
                const char* str = va_arg(args, const char*);
                // Handle NULL string pointer
                if (!str) {
                    str = "(null)";
                }
                while (*str && written < size - 1) {
                    *p++ = *str++;
                    written++;
                }
                f++;
            } else if (*f == '%') {
                // Handle %%
                if (written < size - 1) {
                    *p++ = '%';
                    written++;
                }
                f++;
            } else {
                // Unknown format specifier, treat as literal
                if (written < size - 1) {
                    *p++ = '%';
                    written++;
                }
                if (written < size - 1) {
                    *p++ = *f;
                    written++;
                }
                f++;
            }
        } else {
            *p++ = *f++;
            written++;
        }
    }
    *p = '\0';

    return written;
}