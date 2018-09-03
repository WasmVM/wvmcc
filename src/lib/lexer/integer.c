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

static Token* suffix(char** inputPtr, unsigned long long int value){
    char* cursor = *inputPtr;
    // Suffix
    _Bool isUnsigned = 0;
    unsigned int byteSize = 4;
    if(*cursor == 'u' || *cursor == 'U'){
        ++cursor;
        isUnsigned = 1;
        if((cursor[0] == 'l' && cursor[1] == 'l') || (cursor[0] == 'L' && cursor[1] == 'L')){
            cursor += 2;
            byteSize = 8;
        }else if(*cursor == 'l' || *cursor == 'L'){
            ++cursor;
        }
    }else if((cursor[0] == 'l' && cursor[1] == 'l') || (cursor[0] == 'L' && cursor[1] == 'L')){
        cursor += 2;
        byteSize = 8;
        if(*cursor == 'u' || *cursor == 'U'){
            ++cursor;
            isUnsigned = 1;
        }
    }else if(*cursor == 'l' || *cursor == 'L'){
        ++cursor;
        if(*cursor == 'u' || *cursor == 'U'){
            ++cursor;
            isUnsigned = 1;
        }
    }
    *inputPtr = cursor;
    return new_IntegerToken(value, byteSize, isUnsigned);
}

static Token* decimal(char** inputPtr){
    char* cursor = *inputPtr;
    unsigned long long int value = 0;
    while(*cursor >= '0' && *cursor <= '9'){
        value *= 10;
        value += *cursor - '0';
        ++cursor;
    }
    if(cursor == *inputPtr){
        return NULL;
    }
    *inputPtr = cursor;
    return suffix(inputPtr, value);
}

static Token* hexadecimal(char** inputPtr){
    char* cursor = *inputPtr;
    if(cursor[0] == '0' && (cursor[1] == 'x' || cursor[1] == 'X')){
        cursor += 2;
        unsigned long long int value = 0;
        while((*cursor >= '0' && *cursor <= '9') ||
              (*cursor >= 'a' && *cursor <= 'f') ||
              (*cursor >= 'A' && *cursor <= 'F')){
            value *= 16;
            if(*cursor >= '0' && *cursor <= '9'){
                value += *cursor - '0';
            }else if(*cursor >= 'a' && *cursor <= 'f'){
                value += *cursor - 'a' + 10;
            }else{
                value += *cursor - 'A' + 10;
            }
            ++cursor;
        }
        if(cursor == *inputPtr){
            return NULL;
        }
        *inputPtr = cursor;
        return suffix(inputPtr, value);
    }
    return NULL;
}

static Token* octal(char** inputPtr){
    char* cursor = *inputPtr;
    if(cursor[0] == '0'){
        ++cursor;
        unsigned long long int value = 0;
        while(*cursor >= '0' && *cursor <= '7'){
            value *= 8;
            value += *cursor - '0';
            ++cursor;
        }
        if(cursor == *inputPtr){
            return NULL;
        }
        *inputPtr = cursor;
        return suffix(inputPtr, value);
    }
    return NULL;
}

Token* lex_Integer(char **inputPtr){
    Token* token = NULL;
    if( (token = hexadecimal(inputPtr)) ||
        (token = octal(inputPtr)) ||
        (token = decimal(inputPtr))
    ){
        return token;
    }
    return NULL; 
}