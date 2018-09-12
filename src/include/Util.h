#ifndef WVMCC_UTIL_DEF
#define WVMCC_UTIL_DEF

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <adt/Vector.h>

_Bool isLittleEndian();
uint32_t *to_ustring(const void* str, size_t charSize);
int ustrcmp(const uint32_t* str1, const uint32_t* str2);
uint32_t *ustrcpy(uint32_t* dst, const uint32_t* src);
size_t ustrlen(const uint32_t* src);

#endif