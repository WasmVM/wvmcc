/* LANG-6.2.6.2-06 — 6.2.6.2p6: the width W of an integer type is its precision
 * M plus, for a signed type, one sign bit (W = M + 1); for an unsigned type
 * W == M. With no padding bits (docs/spec.md), W == sizeof(type) * CHAR_BIT.
 * Verify=static-assert. Compile-only; a held assertion = pass. */
#include <limits.h>

_Static_assert(CHAR_BIT == 8, "this test assumes CHAR_BIT == 8");

/* Unsigned: width == precision == number of value bits == sizeof*CHAR_BIT,
 * and max == 2^precision - 1. For unsigned int (32-bit): 2^32 - 1. */
_Static_assert(UINT_MAX == 0xFFFFFFFFu &&
               sizeof(unsigned int) * CHAR_BIT == 32,
               "unsigned int: width == precision == 32");
_Static_assert(ULONG_MAX == 0xFFFFFFFFFFFFFFFFull &&
               sizeof(unsigned long) * CHAR_BIT == 64,
               "unsigned long: width == precision == 64");

/* Signed: precision == width - 1 (one bit is the sign bit). The positive range
 * spans 2^precision - 1, i.e. 2^(width-1) - 1. For int: precision 31. */
_Static_assert(INT_MAX == 0x7FFFFFFF &&
               sizeof(int) * CHAR_BIT == 32,
               "signed int: width 32 == precision 31 + 1 sign bit");
_Static_assert(LONG_MAX == 0x7FFFFFFFFFFFFFFFll &&
               sizeof(long) * CHAR_BIT == 64,
               "signed long: width 64 == precision 63 + 1 sign bit (LP64)");

/* Corresponding signed/unsigned types share the same width; the unsigned type
 * has one more value (precision) bit than its signed counterpart. */
_Static_assert(sizeof(int) == sizeof(unsigned int),
               "signed/unsigned int share a width");
_Static_assert(sizeof(long) == sizeof(unsigned long),
               "signed/unsigned long share a width");
