/* tests/standard/libc/float/flt_max.c — *_MAX.
 * Catalog: LIBC-float-FLT_MAX-01 (ISO C17 5.2.4.2.2p12).
 * Maximum representable finite floating-point number; the standard
 * requires each to be >= 1E37.
 * Verify=static-assert; compile-only, a held assertion = pass. */
#include <float.h>

_Static_assert(FLT_MAX >= 1E37F, "FLT_MAX must be >= 1E37");
_Static_assert(DBL_MAX >= 1E37, "DBL_MAX must be >= 1E37");
_Static_assert(LDBL_MAX >= 1E37L, "LDBL_MAX must be >= 1E37");

/* 6.2.5p10: value-set nesting float <= double <= long double. */
_Static_assert(DBL_MAX >= FLT_MAX, "DBL_MAX >= FLT_MAX");
_Static_assert(LDBL_MAX >= DBL_MAX, "LDBL_MAX >= DBL_MAX");
