#include "genFuncs.h"

int gen_primary_expression_identifier(List *codeList, Token *tok, List *identList){
    // Find identifier
    for(int i = 0; i < identList->size; ++i){
        Identifier *ident = (Identifier *)listAt(identList, i);
        if(!strcmp(ident->declaration->identifier, tok->data.str)){
            if(ident->localidx == -1){
                // Global
                char *newCode = calloc(13 + strlen(tok->data.str),sizeof(char));
                sprintf(newCode, "get_global $%s", tok->data.str);
                listAdd(codeList, newCode);
                return 1;
            }else{
                // TODO: Local
            }
        }
    }
    return 0;
}

void gen_primary_expression_constant(List *codeList, Token *tok){
    if(tok->type == Tok_Float){
        char *newCode = calloc(1090,sizeof(char));
        sprintf(newCode, "f64.const %g", tok->data.floatVal);
        newCode = realloc(newCode, strlen(newCode) + 1);
        listAdd(codeList, newCode);
    }else{
        char *newCode = calloc(1090,sizeof(char));
        sprintf(newCode, "i64.const %lld", tok->data.intVal);
        newCode = realloc(newCode, strlen(newCode) + 1);
        listAdd(codeList, newCode);
    }
}