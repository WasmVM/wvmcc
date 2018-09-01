/**
 *    Copyright 2018 Luis Hsu
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