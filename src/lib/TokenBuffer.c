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