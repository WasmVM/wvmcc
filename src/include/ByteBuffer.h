#ifndef WVMCC_BYTEBUFFER_DEF
#define WVMCC_BYTEBUFFER_DEF

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <adt/Buffer.h>

typedef Buffer ByteBuffer;
ByteBuffer* new_ByteBuffer(size_t size);
int set_ByteBuffer(ByteBuffer* buffer, size_t offset, const void* data, size_t length);

#endif