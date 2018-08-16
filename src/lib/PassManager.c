#include <PassManager.h>

void free_PassManager(PassManager** passManager){
    free(*passManager);
    *passManager = NULL;
}

PassManager* new_PassManager(const Option *option){
    PassManager* passManager = (PassManager*) calloc(1, sizeof(PassManager));
    passManager->passes = NULL;
    passManager->count = 0;
    return passManager;
}