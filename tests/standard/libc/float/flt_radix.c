/* tests/standard/libc/float/flt_radix.c — FLT_RADIX.
 * Catalog: LIBC-float-FLT_RADIX-01 (ISO C17 5.2.4.2.2p11, 7.7p2).
 * FLT_RADIX is the radix of the exponent representation; it shall be a
 * constant expression suitable for use in #if preprocessing directives.
 * IEEE-754 binary formats => 2.
 * Verify=static-assert; compile-only, a held assertion = pass. */
#include <float.h>

/* #if-usability (7.7p2): must be usable in conditional inclusion. */
#if FLT_RADIX != 2
#error "FLT_RADIX must be 2 and usable in #if"
#endif

_Static_assert(FLT_RADIX == 2, "FLT_RADIX must be 2 (IEEE-754 binary)");
_Static_assert(FLT_RADIX >= 2, "FLT_RADIX minimum magnitude is 2");
