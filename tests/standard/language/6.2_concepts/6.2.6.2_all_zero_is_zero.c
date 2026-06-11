/* LANG-6.2.6.2-05 — 6.2.6.2p5: for any integer type, the object representation
 * with all bits zero is a representation of the value 0. Verify=static-assert.
 *
 * Expressed at compile time: the literal 0 cast to each integer type compares
 * equal to that type's zero, and (since two's complement here) the all-bits-set
 * complement of 0 is the maximum unsigned value — confirming 0 is the all-zero
 * pattern. Compile-only; a held assertion = pass. */
#include <limits.h>

_Static_assert((unsigned char)0 == 0, "all-zero unsigned char == 0");
_Static_assert((unsigned short)0 == 0, "all-zero unsigned short == 0");
_Static_assert((unsigned int)0 == 0u, "all-zero unsigned int == 0");
_Static_assert((unsigned long)0 == 0ul, "all-zero unsigned long == 0");
_Static_assert((signed char)0 == 0, "all-zero signed char == 0");
_Static_assert((int)0 == 0, "all-zero int == 0");
_Static_assert((long)0 == 0l, "all-zero long == 0");

/* Complement of all-zero is all-one, i.e. the unsigned max — confirming the
 * zero value corresponds to the all-bits-zero object representation. */
_Static_assert((unsigned int)~0u == UINT_MAX, "~0 == all-ones == UINT_MAX");
_Static_assert((unsigned long)~0ul == ULONG_MAX, "~0 == all-ones == ULONG_MAX");
