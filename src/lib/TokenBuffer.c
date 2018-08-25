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

#include <TokenBuffer.h>

static void add_Token(TokenBuffer* tokenBuffer, Token* token){
    listAdd((List*)tokenBuffer->buffer.data, token);
    ++(tokenBuffer->buffer.length);
}

static void free_TokenBuffer(TokenBuffer** tokenBuffer){
    List* bufferList = (List*)(*tokenBuffer)->buffer.data;
    listFree(&bufferList);
    free(*tokenBuffer);
    *tokenBuffer = NULL;
}

TokenBuffer* new_TokenBuffer(){
    TokenBuffer* tokenBuffer = (TokenBuffer*)malloc(sizeof(TokenBuffer));
    tokenBuffer->buffer.data = listNew();
    tokenBuffer->buffer.length = 0;
    tokenBuffer->buffer.free = (void(*)(Buffer**))free_TokenBuffer;
    tokenBuffer->free = free_TokenBuffer;
    tokenBuffer->add = add_Token;
    return tokenBuffer;
}