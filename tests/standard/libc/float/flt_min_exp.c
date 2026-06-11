/* tests/standard/libc/float/flt_min_exp.c — *_MIN_EXP / *_MAX_EXP.
 * Catalog: LIBC-float-FLT_MIN_EXP-01 (ISO C17 5.2.4.2.2p11).
 * Minimum/maximum binary exponents (FLT_RADIX == 2). IEEE-754 values:
 * binary32: MIN_EXP -125, MAX_EXP +128; binary64: MIN_EXP -1021,
 * MAX_EXP +1024. wvmcc long double aliases double.
 * Verify=static-assert; compile-only, a held assertion = pass. */
#include <float.h>

_Static_assert(FLT_MIN_EXP == -125, "FLT_MIN_EXP (binary32)");
_Static_assert(FLT_MAX_EXP == 128, "FLT_MAX_EXP (binary32)");
_Static_assert(DBL_MIN_EXP == -1021, "DBL_MIN_EXP (binary64)");
_Static_assert(DBL_MAX_EXP == 1024, "DBL_MAX_EXP (binary64)");
_Static_assert(LDBL_MIN_EXP == -1021, "LDBL_MIN_EXP (== double)");
_Static_assert(LDBL_MAX_EXP == 1024, "LDBL_MAX_EXP (== double)");
