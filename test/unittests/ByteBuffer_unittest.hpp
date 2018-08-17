#include <skypat/skypat.h>

#define restrict __restrict__
extern "C"{
    #include <ByteBuffer.h>
    #include <string.h>
}
#undef restrict

SKYPAT_F(ByteBuffer_unittest, create_delete)
{
    ByteBuffer *buffer = new_ByteBuffer(4);
    EXPECT_EQ(buffer->length, 4);
    buffer->free(&buffer);
}