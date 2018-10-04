#include <PassManager.h>

static void freeContext(void *data){
    Context* context = (Context *)data;
    context->free(context);
}

static void add_pass(PassManager* passManager, Pass* pass){
    pass->contextMap = passManager->contextMap;
    listAdd(passManager->passList, pass);
}

void free_PassManager(PassManager** passManager){
    listFree(&(*passManager)->passList);
    mapFree(&(*passManager)->contextMap);
    free(*passManager);
    *passManager = NULL;
}

static int key_compare(void* a, void* b){
    return strcmp((const char*)a, (const char*)b);
}

PassManager* new_PassManager(){
    PassManager* passManager = (PassManager*) calloc(1, sizeof(PassManager));
    passManager->passList = listNew();
    passManager->contextMap = mapNew(key_compare, free, freeContext);
    passManager->addPass = add_pass;
    return passManager;
}

int run_PassManager(PassManager* passManager){
    while(passManager->passList->size > 0){
        Pass* pass = (Pass*)listRemove(passManager->passList, 0);
        int ret = pass->run(pass);
        pass->free(&pass);
        if(ret){
            return ret;
        }
    }
    return 0;
}