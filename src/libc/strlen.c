#include "include/string.h"

size_t strlen(const char * str) {
    if (str == NULL) {
        return -1;
    }
    int ctr = 0;
    while (str[ctr] != '\0') {
        ctr++;
    }
    return ctr;
}
