/* tests/standard/libc/float/decimal_dig.c — DECIMAL_DIG.
 * Catalog: LIBC-float-DECIMAL_DIG-01 (ISO C17 5.2.4.2.2p11).
 * Number of decimal digits n such that any value of the widest supported
 * floating type round-trips through an n-digit decimal representation.
 * Standard minimum: DECIMAL_DIG >= 10. With long double == binary64,
 * DECIMAL_DIG is 17.
 * Verify=static-assert; compile-only, a held assertion = pass. */
#include <float.h>

_Static_assert(DECIMAL_DIG >= 10, "DECIMAL_DIG must be at least 10");
/* Round-trip requirement vs. the widest type's decimal precision. */
_Static_assert(DECIMAL_DIG >= LDBL_DIG, "DECIMAL_DIG >= LDBL_DIG");
