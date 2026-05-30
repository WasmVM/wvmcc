// M2-1: <stddef.h>.
#ifndef _WVMCC_STDDEF_H
#define _WVMCC_STDDEF_H

// NOTE: must be #define, not typedef — wvmcc's codegen has a known
// gap where typedef-name resolution doesn't propagate to the WasmVM
// value-type mapping, so `typedef unsigned long size_t;` ends up
// sizing `size_t` as i32 rather than i64. Same story for ptrdiff_t.
#define size_t    unsigned long   // wasm64: 8 bytes (matches sizeof(void*))
#define ptrdiff_t long
#define wchar_t   int             // placeholder — wvmcc has no wide-char support

#define NULL ((void*)0)

// `__builtin_offsetof` isn't implemented in wvmcc yet (M2-A series); fall
// back to the pre-C11 trick of computing the offset from a null base.
// Strict-aliasing UB in theory; works on every actual compiler.
#define offsetof(T, m) ((size_t)&(((T*)0)->m))

#endif // _WVMCC_STDDEF_H
