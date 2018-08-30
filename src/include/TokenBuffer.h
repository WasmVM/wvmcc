#ifndef WVMCC_TOKENBUFFER_DEF
#define WVMCC_TOKENBUFFER_DEF

#include <stdlib.h>
#include <adt/List.h>
#include <adt/Token.h>
#include <adt/Buffer.h>

typedef struct _TokenBuffer{
    Buffer buffer;
    void (*add)(struct _TokenBuffer* tokenBuffer, Token* token);
    void (*free)(struct _TokenBuffer** tokenBuffer);
} TokenBuffer;

TokenBuffer* new_TokenBuffer();
#endif