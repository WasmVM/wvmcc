/* tests/standard/libc/stddef/ptrdiff_t.c — LIBC-stddef-ptrdiff_t-01 (C17 7.19p2).
 * Verify=static-assert. ptrdiff_t is the signed integer type of the result
 * of subtracting two pointers. B-impl: wvmcc documents LP64 (64-bit). */
#include <stddef.h>

/* Signed: -1 must remain negative. */
_Static_assert((ptrdiff_t)-1 < 0, "ptrdiff_t is signed");

/* Pointer subtraction yields a value of type ptrdiff_t (6.5.6p9). */
_Static_assert(_Generic((char *)0 - (char *)0, ptrdiff_t: 1, default: 0),
               "pointer difference has type ptrdiff_t");

/* Documented implementation choice (LP64): ptrdiff_t is 64-bit. */
_Static_assert(sizeof(ptrdiff_t) == 8, "LP64: sizeof(ptrdiff_t) == 8");
