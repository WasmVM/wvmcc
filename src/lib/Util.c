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

#include <Util.h>

uint32_t *to_ustring(const char* str){
    size_t len = strlen(str);
    uint32_t *ustr = (uint32_t*)calloc(len + 1, sizeof(uint32_t));
    for(size_t i = 0; i < len; ++i){
        ustr[i] = str[i];
    }
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