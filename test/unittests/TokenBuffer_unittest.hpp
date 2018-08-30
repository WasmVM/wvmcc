#include <skypat/skypat.h>

#define restrict __restrict__
extern "C"{
    #include <TokenBuffer.h>
    #include <string.h>
}
#undef restrict

SKYPAT_F(TokenBuffer, create_delete)
{
    TokenBuffer* tokenBuffer = new_TokenBuffer();
    EXPECT_EQ(tokenBuffer->buffer.length, 0);
    tokenBuffer->free(&tokenBuffer);
    EXPECT_EQ(tokenBuffer, NULL);
}

SKYPAT_F(TokenBuffer, polymorphism)
{
    Buffer* buffer = (Buffer*)new_TokenBuffer();
    EXPECT_EQ(buffer->length, 0);
    buffer->free(&buffer);
    EXPECT_EQ(buffer, NULL);
}

SKYPAT_F(TokenBuffer, add_token)
{
    TokenBuffer* tokenBuffer = (TokenBuffer*)new_TokenBuffer();
    Token* token = new_UnknownToken();
    tokenBuffer->add(tokenBuffer, token);
    EXPECT_EQ(tokenBuffer->buffer.length, 1);
    EXPECT_EQ(((List*)(tokenBuffer->buffer.data))->size, 1);
    tokenBuffer->free(&tokenBuffer);
}