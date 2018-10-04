#include <Lexer.h>

Token* lex_Identifier(char **inputPtr){
    char *cursor = *inputPtr;
    if(*cursor == '_' ||
        (*cursor >= 'A' && *cursor <= 'Z') ||
        (*cursor >= 'a' && *cursor <= 'z')
    ){
        do{
            ++cursor;
        }while(*cursor == '_' ||
            (*cursor >= 'A' && *cursor <= 'Z') ||
            (*cursor >= 'a' && *cursor <= 'z') ||
            (*cursor >= '0' && *cursor <= '9')
        );
        size_t length = cursor - *inputPtr;
        char* matched = (char*) calloc(length + 1, sizeof(char));
        strncpy(matched, *inputPtr, length);
        *inputPtr += length;
        return new_IdentifierToken(matched);
    }
    return NULL;
}