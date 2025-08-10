#include <stdint.h>
#include "include/string.h"

void* memset(void* ptr, int value, size_t num) {
    uint8_t* arr = (uint8_t*)ptr;
    for (size_t i = 0; i < num; i++) {
        arr[i] = value;
    }
    return ptr;
}