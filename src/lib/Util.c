#include <Util.h>

_Bool isLittleEndian(){
    int one = 1;
    return *(char*)&one;
}

static uint32_t getUchar(const uint8_t* str, size_t charSize){
    uint32_t value = 0;
    if(isLittleEndian()){
        for(size_t i = charSize; i > 0; --i){
            value <<= 8;
            value += str[i - 1];
        }
    }else{
        for(size_t i = 0; i < charSize; ++i){
            value <<= 8;
            value += str[i - 1];
        }
    }
    return value;
}

uint32_t *to_ustring(const void* str, size_t charSize){
    Vector* strVec = new_Vector(sizeof(uint32_t), NULL);
    uint32_t uChar = 0;
    while((uChar = getUchar((uint8_t*)str, charSize)) != 0){
        strVec->push_back(strVec, &uChar);
        str += charSize;
    }
    strVec->push_back(strVec, &uChar);
    uint32_t* ustr = (uint32_t*) malloc(sizeof(uint32_t) * strVec->length);
    memcpy(ustr, strVec->data, sizeof(uint32_t) * strVec->length);
    return ustr;
}

int ustrcmp(const uint32_t* str1, const uint32_t* str2){
    while(*str1 && *str2 && *str1 == *str2){
        ++str1;
        ++str2;
    }
    return (*str1 > *str2) ? 1 : ((*str1 == *str2) ? 0 : -1);
}

uint32_t *ustrcpy(uint32_t* dst, const uint32_t* src){
    for(size_t i = 0; i < ustrlen(src); ++i){
        dst[i] = src[i];
    }
    return dst;
}

size_t ustrlen(const uint32_t* src){
    size_t len = 0;
    while(src[len] != 0){
        ++len;
    }
    return len;
}