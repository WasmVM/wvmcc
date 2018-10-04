#include <adt/Vector.h>

static void _push_back(Vector* vector, const void* valuePtr){
    if(vector->length >= vector->capacity){
        if(vector->length == 0){
            vector->data = malloc(vector->unitSize);
            vector->capacity = 1;
        }else{
            vector->data = realloc(vector->data, vector->unitSize * vector->length * 2);
            vector->capacity = vector->length * 2;
        }
    }
    memcpy(vector->data + (vector->unitSize * vector->length), valuePtr, vector->unitSize);
    ++vector->length;
}
static void* _pop_back(Vector* vector){
    if(vector->length <= 0){
        return NULL;
    }
    void* valuePtr = vector->at(vector, vector->length - 1);
    --vector->length;
    return valuePtr;
}
static void _shrink(Vector* vector){
    if(vector->length < vector->capacity){
        vector->data = realloc(vector->data, vector->unitSize * vector->length);
        vector->capacity = vector->length;
    }
}
static void* _at(Vector* vector, size_t index){
    if(vector->length <= 0 || index >= vector->length) {
        return NULL;
    }
    void* result = malloc(vector->unitSize);
    memcpy(result, vector->data + (index * vector->unitSize), vector->unitSize);
    return result;
}

Vector* new_Vector(size_t unitSize, void (*freeElem)(void*)){
    Vector* vector = (Vector*) malloc(sizeof(Vector));
    vector->data = NULL;
    vector->length = 0;
    vector->capacity = 0;
    vector->unitSize = unitSize;
    vector->freeElem = freeElem;
    vector->push_back = _push_back;
    vector->pop_back = _pop_back;
    vector->shrink = _shrink;
    vector->at = _at;
    return vector;
}
void free_Vector(Vector** vectorPtr){
    Vector* vector = *vectorPtr;
    if(vector->freeElem){
        for(size_t i = 0; i < vector->length; ++i){
            vector->freeElem(vector->at(vector, i));
        }
    }
    free(vector->data);
    free(vector);
    *vectorPtr = NULL;
}