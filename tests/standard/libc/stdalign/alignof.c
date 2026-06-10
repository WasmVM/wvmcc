/* tests/standard/libc/stdalign/alignof.c — <stdalign.h> alignof macro.
 * Catalog: LIBC-stdalign-alignof-01 (docs/standard/libc.md). Verify=static-assert.
 * ISO C17 7.15p2: the macro `alignof` expands to `_Alignof`.
 * 6.5.3.4p3 / 6.2.8: _Alignof yields the alignment requirement of its type,
 * an integer constant expression.
 * Compile-only (-ffreestanding); a held assertion = pass. */
#include <stdalign.h>

#ifndef alignof
#error "alignof not defined as a macro"
#endif

/* alignof must behave exactly like _Alignof and be usable in an ICE. */
_Static_assert(alignof(char) == 1, "alignof(char) == 1 (6.2.8p2)");
_Static_assert(alignof(char) == _Alignof(char), "alignof expands to _Alignof");
_Static_assert(alignof(int) == _Alignof(int), "alignof expands to _Alignof");
_Static_assert(alignof(long) == _Alignof(long), "alignof expands to _Alignof");

/* Alignment is a divisor of the type's size (6.2.8: objects of the type can
 * be placed at any address that is a multiple of the alignment, and array
 * elements are contiguous). */
_Static_assert(sizeof(int) % alignof(int) == 0, "alignof(int) divides sizeof(int)");
_Static_assert(sizeof(long) % alignof(long) == 0, "alignof(long) divides sizeof(long)");
