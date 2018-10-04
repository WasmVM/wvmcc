#include <ByteBuffer.h>

static void free_ByteBuffer(ByteBuffer** buffer){
    free((*buffer)->data);
    free(*buffer);
    *buffer = NULL;
}

ByteBuffer* new_ByteBuffer(size_t size){
    Buffer* buffer = (Buffer*) calloc(1, sizeof(Buffer));
    if(size > 0){
        buffer->data = calloc(size, sizeof(char));
    }else{
        buffer->data = NULL;
    }
    buffer->free = free_ByteBuffer;
    buffer->length = size;
    return buffer;
}

int set_ByteBuffer(ByteBuffer* buffer, size_t offset, const void* data, size_t length){
    if(offset + length > buffer->length){
        return -1;
    }
    memcpy(buffer->data + offset, data, length);
    return 0;
}