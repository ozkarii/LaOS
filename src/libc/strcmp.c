#include "include/string.h"

int strcmp ( const char * str1, const char * str2 ) {
    if (str1 == NULL || str2 == NULL) {
        return -1;
    }
    int i = 0;
    while (str1[i] != '\0' && str2[i] != '\0') {
        int res = (unsigned char)str1[i] - (unsigned char)str2[i];
        if (res != 0) {
            return res;
        }
        i++;
    }
    return (unsigned char)str1[i] - (unsigned char)str2[i];
}