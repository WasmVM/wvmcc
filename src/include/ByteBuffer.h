#ifndef WVMCC_BYTEBUFFER_DEF
#define WVMCC_BYTEBUFFER_DEF

#include <stddef.h>
#include <stdlib.h>
#include <adt/Buffer.h>

typedef Buffer ByteBuffer;
ByteBuffer* new_ByteBuffer(size_t size);

#endif