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