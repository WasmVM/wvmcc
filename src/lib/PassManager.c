/**
 *    Copyright 2017 Luis Hsu
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

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
        for(size_t i = 0; i < pass->input_count; ++i){
            pass->input[i]->free(&(pass->input[i]));
        }
        pass->free(&pass);
        if(ret){
            return ret;
        }
    }
    return 0;
}