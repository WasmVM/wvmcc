#ifndef WVMCC_PASSMANAGER_DEF
#define WVMCC_PASSMANAGER_DEF

#include <stddef.h>
#include <stdlib.h>
#include <Option.h>
#include <Pass.h>

typedef struct {
    size_t count;
    Pass* passes;
} PassManager;

PassManager* new_PassManager();
void free_PassManager(PassManager** passManager);

#endif