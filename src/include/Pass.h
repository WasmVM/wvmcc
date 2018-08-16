#ifndef WVMCC_PASS_DEF
#define WVMCC_PASS_DEF

#include <stddef.h>
#include <Buffer.h>

typedef struct _Pass{
    Buffer** input;
    size_t input_count;
    Buffer** output;
    size_t output_count;
    int (*run)(struct _Pass* pass);
} Pass;

#endif