#include <skypat/skypat.h>

#define restrict __restrict__
extern "C"{
    #include <TokenBuffer.h>
    #include <string.h>
}
#undef restrict

SKYPAT_F(TokenBuffer_unittest, create_delete)
{
    TokenBuffer* tokenBuffer = new_TokenBuffer();
    EXPECT_EQ(tokenBuffer->buffer.length, 0);
    tokenBuffer->free(&tokenBuffer);
    EXPECT_EQ(tokenBuffer, NULL);
}

SKYPAT_F(TokenBuffer_unittest, polymorphism)
{
    Buffer* tokenBuffer = (Buffer*)new_TokenBuffer();
    EXPECT_EQ(tokenBuffer->length, 0);
    tokenBuffer->free(&tokenBuffer);
    EXPECT_EQ(tokenBuffer, NULL);
}