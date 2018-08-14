#ifndef WVMCC_PASSMANAGER_DEF
#define WVMCC_PASSMANAGER_DEF

#include <Pass.h>

typedef struct {
    Pass* passes;
    size_t passCount;
    int (*run)();
} PassManager;

PassManager* newPassManager();

#endif