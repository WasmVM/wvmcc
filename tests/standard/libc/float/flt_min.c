/* tests/standard/libc/float/flt_min.c — *_MIN.
 * Catalog: LIBC-float-FLT_MIN-01 (ISO C17 5.2.4.2.2p13).
 * Minimum normalized positive floating-point number; the standard
 * requires each to be <= 1E-37.
 * Verify=static-assert; compile-only, a held assertion = pass. */
#include <float.h>

_Static_assert(FLT_MIN <= 1E-37F, "FLT_MIN must be <= 1E-37");
_Static_assert(DBL_MIN <= 1E-37, "DBL_MIN must be <= 1E-37");
_Static_assert(LDBL_MIN <= 1E-37L, "LDBL_MIN must be <= 1E-37");

_Static_assert(FLT_MIN > 0, "FLT_MIN must be positive");
_Static_assert(DBL_MIN > 0, "DBL_MIN must be positive");
_Static_assert(LDBL_MIN > 0, "LDBL_MIN must be positive");
