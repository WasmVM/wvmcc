/* tests/standard/libc/stdalign/alignas.c — <stdalign.h> alignas macro.
 * Catalog: LIBC-stdalign-alignas-01 (docs/standard/libc.md). Verify=static-assert.
 * ISO C17 7.15p2: the macro `alignas` expands to `_Alignas`.
 * Compile-only (-ffreestanding); a held assertion = pass. */
#include <stdalign.h>

#ifndef alignas
#error "alignas not defined as a macro"
#endif

/* alignas must be usable wherever _Alignas is: with a constant expression
 * (6.7.5p3) and with a type name. The declarations must compile. */
static alignas(8) char aligned_by_const[8];
static alignas(long) char aligned_by_type[8];

_Static_assert(sizeof(aligned_by_const) == 8, "array size unaffected by alignas");
_Static_assert(sizeof(aligned_by_type) == 8, "array size unaffected by alignas");

/* alignas inside a struct member declaration (6.7.5p2): the struct must be
 * at least as large as the over-aligned member's alignment requires. */
struct over_aligned {
    alignas(8) char c;
};
_Static_assert(sizeof(struct over_aligned) >= 8,
               "alignas(8) member forces struct size >= 8");
_Static_assert(_Alignof(struct over_aligned) == 8,
               "alignas(8) member forces struct alignment 8");
