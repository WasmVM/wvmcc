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