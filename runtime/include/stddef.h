// M2-1: <stddef.h>.
#ifndef _WVMCC_STDDEF_H
#define _WVMCC_STDDEF_H

typedef unsigned long size_t;    // wasm64: 8 bytes (matches sizeof(void*))
typedef long          ptrdiff_t;
typedef int           wchar_t;   // placeholder — wvmcc has no wide-char support

#define NULL ((void*)0)

// `__builtin_offsetof` isn't implemented in wvmcc yet (M2-A series); fall
// back to the pre-C11 trick of computing the offset from a null base.
// Strict-aliasing UB in theory; works on every actual compiler.
#define offsetof(T, m) ((size_t)&(((T*)0)->m))

#endif // _WVMCC_STDDEF_H
