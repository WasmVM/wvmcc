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

// `offsetof` yields a size_t integer *constant* (7.19), so it must be usable in
// an ICE (_Static_assert, case labels, array bounds). wvmcc's `__builtin_offsetof`
// computes the offset at translation time directly from the type's layout — the
// old `((size_t)&(((T*)0)->m))` trick is not an ICE (6.6p6: a pointer cast /
// address is not an integer-constant-expression operand).
#define offsetof(T, m) __builtin_offsetof(T, m)

#endif // _WVMCC_STDDEF_H
