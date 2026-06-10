/* tests/standard/libc/float/flt_dig.c — *_DIG.
 * Catalog: LIBC-float-FLT_DIG-01 (ISO C17 5.2.4.2.2p11).
 * Number of decimal digits q such that any q-digit decimal number can be
 * rounded into the type and back without change. Standard minimums:
 * FLT_DIG >= 6, DBL_DIG >= 10, LDBL_DIG >= 10.
 * Verify=static-assert; compile-only, a held assertion = pass. */
#include <float.h>

_Static_assert(FLT_DIG >= 6, "FLT_DIG must be at least 6");
_Static_assert(DBL_DIG >= 10, "DBL_DIG must be at least 10");
_Static_assert(LDBL_DIG >= 10, "LDBL_DIG must be at least 10");
_Static_assert(LDBL_DIG >= DBL_DIG, "LDBL_DIG >= DBL_DIG");
_Static_assert(DBL_DIG >= FLT_DIG, "DBL_DIG >= FLT_DIG");
