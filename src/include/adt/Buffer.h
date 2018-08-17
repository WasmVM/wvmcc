#ifndef WVMCC_BUFFER_DEF
#define WVMCC_BUFFER_DEF

#include <stddef.h>

typedef struct _Buffer{
    void* data;
    size_t length;
    void (*free)(struct _Buffer**);
} Buffer;

#endif