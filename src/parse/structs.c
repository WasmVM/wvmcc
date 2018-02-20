#include "structs.h"

void initType(Type *type){
    type->storage = 0;
    type->specifier = 0;
    type->qualifier = 0;
}

void freeType(Type *type){
    switch(type->specifier){
        case Type_Struct:
        case Type_Union:{
            StructUnion *typePtr = (StructUnion *)type;
            if(typePtr->props){
                while(typePtr->props->size > 0){
                    freeType(listAt(typePtr->props, 0));
                }
                listFree(&typePtr->props);
            }
            free(typePtr);
        }break;
        default:
            free(type);
    }
}