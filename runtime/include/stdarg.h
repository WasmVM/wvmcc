// M2-1: <stdarg.h>.
//
// Thin wrappers over the M2-A `__builtin_va_*` intrinsics. `va_list` is
// `#define`d (not `typedef`'d) because the typedef-name resolution in
// the codegen type system doesn't follow typedef chains all the way down
// to the wasm value type — `typedef __builtin_va_list va_list; va_list
// ap;` mis-sizes `ap` as i32. See [[reference_typedef_resolution_gap]].
#ifndef _WVMCC_STDARG_H
#define _WVMCC_STDARG_H

#define va_list            __builtin_va_list
#define va_start(ap, last) __builtin_va_start(ap, last)
#define va_arg(ap, T)      __builtin_va_arg(ap, T)
#define va_end(ap)         __builtin_va_end(ap)
#define va_copy(dst, src)  __builtin_va_copy(dst, src)

#endif // _WVMCC_STDARG_H
