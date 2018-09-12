#ifndef WVMCC_VECTOR_DEF
#define WVMCC_VECTOR_DEF

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

typedef struct _Vector{
    void *data;
    size_t length;
    size_t capacity;
    size_t unitSize;
    void (*freeElem)(void* elem);
    void (*push_back)(struct _Vector* vector, const void* value);
    void* (*pop_back)(struct _Vector* vector);
    void (*shrink)(struct _Vector* vector);
    void* (*at)(struct _Vector* vector, size_t index);
} Vector;

Vector* new_Vector(size_t unitSize, void (*freeElem)(void* elem));
void free_Vector(Vector** vectorPtr);

#endif