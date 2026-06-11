/* LANG-5.2.4.2.2-02 — 5.2.4.2.2: the actual floating-type characteristics are
 * implementation-defined.  docs/spec.md documents IEEE-754: float is binary32,
 * double is binary64, and long double aliases double. */
#include <float.h>

_Static_assert(FLT_RADIX == 2, "FLT_RADIX == 2 (binary IEEE-754)");

/* float == IEEE-754 binary32 */
_Static_assert(FLT_MANT_DIG == 24, "FLT_MANT_DIG == 24");
_Static_assert(FLT_DIG == 6, "FLT_DIG == 6");
_Static_assert(FLT_MIN_EXP == -125, "FLT_MIN_EXP == -125");
_Static_assert(FLT_MAX_EXP == 128, "FLT_MAX_EXP == 128");
_Static_assert(FLT_MIN_10_EXP == -37, "FLT_MIN_10_EXP == -37");
_Static_assert(FLT_MAX_10_EXP == 38, "FLT_MAX_10_EXP == 38");

/* double == IEEE-754 binary64 */
_Static_assert(DBL_MANT_DIG == 53, "DBL_MANT_DIG == 53");
_Static_assert(DBL_DIG == 15, "DBL_DIG == 15");
_Static_assert(DBL_MIN_EXP == -1021, "DBL_MIN_EXP == -1021");
_Static_assert(DBL_MAX_EXP == 1024, "DBL_MAX_EXP == 1024");
_Static_assert(DBL_MIN_10_EXP == -307, "DBL_MIN_10_EXP == -307");
_Static_assert(DBL_MAX_10_EXP == 308, "DBL_MAX_10_EXP == 308");

/* long double aliases double (docs/spec.md) */
_Static_assert(LDBL_MANT_DIG == DBL_MANT_DIG, "long double aliases double: mantissa");
_Static_assert(LDBL_DIG == DBL_DIG, "long double aliases double: digits");
_Static_assert(LDBL_MIN_EXP == DBL_MIN_EXP, "long double aliases double: min exp");
_Static_assert(LDBL_MAX_EXP == DBL_MAX_EXP, "long double aliases double: max exp");
_Static_assert(LDBL_MIN_10_EXP == DBL_MIN_10_EXP, "long double aliases double: min 10 exp");
_Static_assert(LDBL_MAX_10_EXP == DBL_MAX_10_EXP, "long double aliases double: max 10 exp");
