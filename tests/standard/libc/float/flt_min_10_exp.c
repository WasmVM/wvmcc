/* tests/standard/libc/float/flt_min_10_exp.c — *_MIN_10_EXP / *_MAX_10_EXP.
 * Catalog: LIBC-float-FLT_MIN_10_EXP-01 (ISO C17 5.2.4.2.2p11).
 * Minimum/maximum decimal exponents. Standard requires *_MIN_10_EXP <= -37
 * and *_MAX_10_EXP >= +37 for every floating type.
 * Verify=static-assert; compile-only, a held assertion = pass. */
#include <float.h>

_Static_assert(FLT_MIN_10_EXP <= -37, "FLT_MIN_10_EXP must be <= -37");
_Static_assert(DBL_MIN_10_EXP <= -37, "DBL_MIN_10_EXP must be <= -37");
_Static_assert(LDBL_MIN_10_EXP <= -37, "LDBL_MIN_10_EXP must be <= -37");

_Static_assert(FLT_MAX_10_EXP >= 37, "FLT_MAX_10_EXP must be >= +37");
_Static_assert(DBL_MAX_10_EXP >= 37, "DBL_MAX_10_EXP must be >= +37");
_Static_assert(LDBL_MAX_10_EXP >= 37, "LDBL_MAX_10_EXP must be >= +37");
