#ifndef WVMCC_PASSMANAGER_DEF
#define WVMCC_PASSMANAGER_DEF

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <adt/list.h>
#include <adt/map.h>
#include <adt/Pass.h>
#include <adt/Context.h>

typedef struct _PassManager{
    List* passList;
    Map* contextMap;
    void (*addPass)(struct _PassManager* passManager, Pass* pass);
} PassManager;

PassManager* new_PassManager();
void free_PassManager(PassManager** passManager);
int run_PassManager(PassManager* passManager);

#endif