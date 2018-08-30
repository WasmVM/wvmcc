#ifndef WVMCC_PASS_DEF
#define WVMCC_PASS_DEF

#include <stddef.h>
#include <adt/Buffer.h>
#include <adt/Map.h>

typedef struct _Pass{
    Buffer** input;
    size_t input_count;
    Buffer** output;
    size_t output_count
    Map* contextMap;
    int (*run)(struct _Pass* pass);
    void (*free)(struct _Pass** pass);
} Pass;

#endif