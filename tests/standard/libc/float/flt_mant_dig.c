/* tests/standard/libc/float/flt_mant_dig.c — *_MANT_DIG.
 * Catalog: LIBC-float-FLT_MANT_DIG-01 (ISO C17 5.2.4.2.2p11).
 * Number of base-FLT_RADIX digits in the significand.
 * IEEE-754 binary32 => 24; binary64 => 53; wvmcc long double aliases
 * double (docs/spec.md) => 53.
 * Verify=static-assert; compile-only, a held assertion = pass. */
#include <float.h>

_Static_assert(FLT_MANT_DIG == 24, "FLT_MANT_DIG (binary32) must be 24");
_Static_assert(DBL_MANT_DIG == 53, "DBL_MANT_DIG (binary64) must be 53");
_Static_assert(LDBL_MANT_DIG == 53, "LDBL_MANT_DIG (== double) must be 53");

/* 6.2.5p10: the set of values of double is a subset of long double. */
_Static_assert(LDBL_MANT_DIG >= DBL_MANT_DIG,
               "LDBL_MANT_DIG >= DBL_MANT_DIG");
_Static_assert(DBL_MANT_DIG >= FLT_MANT_DIG,
               "DBL_MANT_DIG >= FLT_MANT_DIG");
