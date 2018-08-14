#ifndef WVMCC_BUFFER_DEF
#define WVMCC_BUFFER_DEF

#include <stddef.h>

typedef struct {
    void* data;
    size_t length;
    void (*free)();
} Buffer;

#endif