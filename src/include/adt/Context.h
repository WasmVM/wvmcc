#ifndef WVMCC_CONTEXT_DEF
#define WVMCC_CONTEXT_DEF

typedef struct _Context{
    void *data;
    void (*free)(struct _Context* context);
} Context;

#endif