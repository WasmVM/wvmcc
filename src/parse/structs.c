#include "structs.h"

void initDeclaration(Declaration *decl){
    decl->storage = 0;
    decl->specifier = 0;
    decl->qualifier = 0;
    decl->declarators = listNew();
    decl->identifier = NULL;
}

void freeDeclaration(Declaration *decl){
    free(decl->identifier);
    listFree(&(decl->declarators));
    switch(decl->specifier){
        case Type_Struct:
        case Type_Union:{
            StructUnion *declPtr = (StructUnion *)decl;
            if(declPtr->props){
                while(declPtr->props->size > 0){
                    freeDeclaration(listAt(declPtr->props, 0));
                }
                listFree(&declPtr->props);
            }
            free(declPtr);
        }break;
        default:
            free(decl);
    }
}