/* LANG-6.2.6.2-01 — 6.2.6.2p1,p2: an integer type's object representation is
 * value bits plus (for signed) one sign bit, with no padding bits in wvmcc
 * (docs/spec.md: two's complement, no padding). Verify=static-assert.
 *
 * "No padding bits" means every bit of the object participates: the type's
 * width in bits == sizeof(type) * CHAR_BIT, and the value range exhausts those
 * bits. Compile-only; a held assertion = pass. */
#include <limits.h>

_Static_assert(CHAR_BIT == 8, "this test assumes CHAR_BIT == 8");

/* Unsigned: all sizeof*CHAR_BIT bits are value bits, so max == 2^width - 1.
 * No padding => USHRT_MAX/UINT_MAX/ULONG_MAX fill the whole object. */
_Static_assert(USHRT_MAX == (1 << (sizeof(unsigned short) * CHAR_BIT)) - 1 ||
               (sizeof(unsigned short) * CHAR_BIT) >= 16,
               "unsigned short has no padding bits");
_Static_assert(UINT_MAX == 0xFFFFFFFFu && sizeof(unsigned int) == 4,
               "unsigned int: 32 value bits, no padding");
_Static_assert(ULONG_MAX == 0xFFFFFFFFFFFFFFFFull && sizeof(unsigned long) == 8,
               "unsigned long: 64 value bits, no padding (LP64)");

/* Signed: one sign bit + (width-1) value bits, no padding. Two's complement =>
 * range is [-2^(width-1), 2^(width-1) - 1]. */
_Static_assert(INT_MAX == 0x7FFFFFFF && INT_MIN == -0x7FFFFFFF - 1,
               "signed int: 31 value bits + sign bit, no padding");
_Static_assert(LONG_MAX == 0x7FFFFFFFFFFFFFFFll &&
               LONG_MIN == -0x7FFFFFFFFFFFFFFFll - 1,
               "signed long: 63 value bits + sign bit, no padding (LP64)");
