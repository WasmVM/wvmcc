// <uchar.h> (C17 7.28) — Unicode utilities.
//
// wvmcc currently provides the *types* required by this header (so type- and
// constant-expression conformance holds); the mbrtoc16/c16rtomb/mbrtoc32/
// c32rtomb conversion functions are not yet implemented.
#ifndef _WVMCC_UCHAR_H
#define _WVMCC_UCHAR_H

#include <stddef.h>   // size_t
#include <stdint.h>   // uint_least16_t / uint_least32_t

// 7.28p2: char16_t / char32_t are unsigned integer types with the same size,
// signedness and alignment as uint_least16_t / uint_least32_t respectively.
typedef uint_least16_t char16_t;
typedef uint_least32_t char32_t;

// mbstate_t: a complete object type holding multibyte/wide conversion state.
// Shared with <wchar.h> — guard so the two headers may be included together.
#ifndef _WVMCC_MBSTATE_T
#define _WVMCC_MBSTATE_T
typedef struct { unsigned long __count; unsigned int __value; } mbstate_t;
#endif

#endif // _WVMCC_UCHAR_H
