/* tests/standard/libc/float/flt_epsilon.c — *_EPSILON.
 * Catalog: LIBC-float-FLT_EPSILON-01 (ISO C17 5.2.4.2.2p13).
 * Difference between 1 and the least value greater than 1 representable
 * in the type. Standard maximums: FLT_EPSILON <= 1E-5,
 * DBL_EPSILON <= 1E-9, LDBL_EPSILON <= 1E-9.
 * Verify=static-assert; compile-only, a held assertion = pass. */
#include <float.h>

_Static_assert(FLT_EPSILON <= 1E-5F, "FLT_EPSILON must be <= 1E-5");
_Static_assert(DBL_EPSILON <= 1E-9, "DBL_EPSILON must be <= 1E-9");
_Static_assert(LDBL_EPSILON <= 1E-9L, "LDBL_EPSILON must be <= 1E-9");

_Static_assert(FLT_EPSILON > 0, "FLT_EPSILON must be positive");
_Static_assert(DBL_EPSILON > 0, "DBL_EPSILON must be positive");
_Static_assert(LDBL_EPSILON > 0, "LDBL_EPSILON must be positive");

/* 1 + EPSILON must be distinguishable from 1 in the type. */
_Static_assert(1.0F + FLT_EPSILON > 1.0F, "1+FLT_EPSILON > 1");
_Static_assert(1.0 + DBL_EPSILON > 1.0, "1+DBL_EPSILON > 1");
_Static_assert(1.0L + LDBL_EPSILON > 1.0L, "1+LDBL_EPSILON > 1");
