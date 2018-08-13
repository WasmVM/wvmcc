#ifndef WVMCC_PASS_DEF
#define WVMCC_PASS_DEF

#include <stddef.h>
#include <Buffer.h>

typedef struct {
    void* context;
    void* args;
    int (*run)(Buffer* input, Buffer* output);
    size_t index;
} Pass;

#endif