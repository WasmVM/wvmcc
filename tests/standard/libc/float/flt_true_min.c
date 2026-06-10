/* tests/standard/libc/float/flt_true_min.c — *_TRUE_MIN.
 * Catalog: LIBC-float-FLT_TRUE_MIN-01 (ISO C17 5.2.4.2.2p13).
 * Minimum positive floating-point number (subnormal when supported);
 * the standard requires each to be <= 1E-37 and positive, and it cannot
 * exceed the corresponding normalized minimum *_MIN.
 * Verify=static-assert; compile-only, a held assertion = pass. */
#include <float.h>

_Static_assert(FLT_TRUE_MIN > 0, "FLT_TRUE_MIN must be positive");
_Static_assert(DBL_TRUE_MIN > 0, "DBL_TRUE_MIN must be positive");
_Static_assert(LDBL_TRUE_MIN > 0, "LDBL_TRUE_MIN must be positive");

_Static_assert(FLT_TRUE_MIN <= 1E-37F, "FLT_TRUE_MIN must be <= 1E-37");
_Static_assert(DBL_TRUE_MIN <= 1E-37, "DBL_TRUE_MIN must be <= 1E-37");
_Static_assert(LDBL_TRUE_MIN <= 1E-37L, "LDBL_TRUE_MIN must be <= 1E-37");

_Static_assert(FLT_TRUE_MIN <= FLT_MIN, "FLT_TRUE_MIN <= FLT_MIN");
_Static_assert(DBL_TRUE_MIN <= DBL_MIN, "DBL_TRUE_MIN <= DBL_MIN");
_Static_assert(LDBL_TRUE_MIN <= LDBL_MIN, "LDBL_TRUE_MIN <= LDBL_MIN");
