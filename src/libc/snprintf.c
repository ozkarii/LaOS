#include "include/stdio.h"

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
