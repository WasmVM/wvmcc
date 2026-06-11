/* LANG-5.2.4.2.2-01 — 5.2.4.2.2p7: the integer-valued <float.h> macros expand
 * to integer constant expressions suitable for use in #if directives, with
 * values (magnitudes) meeting the standard minimums. */
#include <float.h>

/* #if-usability. */
#if FLT_RADIX < 2
#error "FLT_RADIX below minimum"
#endif
#if FLT_DIG < 6 || DBL_DIG < 10 || LDBL_DIG < 10
#error "*_DIG below minimum"
#endif
#if FLT_MAX_10_EXP < 37 || DBL_MAX_10_EXP < 37 || LDBL_MAX_10_EXP < 37
#error "*_MAX_10_EXP below minimum"
#endif

_Static_assert(FLT_RADIX >= 2, "FLT_RADIX >= 2");

_Static_assert(FLT_DIG >= 6, "FLT_DIG >= 6");
_Static_assert(DBL_DIG >= 10, "DBL_DIG >= 10");
_Static_assert(LDBL_DIG >= 10, "LDBL_DIG >= 10");

_Static_assert(FLT_MIN_10_EXP <= -37, "FLT_MIN_10_EXP <= -37");
_Static_assert(DBL_MIN_10_EXP <= -37, "DBL_MIN_10_EXP <= -37");
_Static_assert(LDBL_MIN_10_EXP <= -37, "LDBL_MIN_10_EXP <= -37");

_Static_assert(FLT_MAX_10_EXP >= 37, "FLT_MAX_10_EXP >= 37");
_Static_assert(DBL_MAX_10_EXP >= 37, "DBL_MAX_10_EXP >= 37");
_Static_assert(LDBL_MAX_10_EXP >= 37, "LDBL_MAX_10_EXP >= 37");

_Static_assert(DECIMAL_DIG >= 10, "DECIMAL_DIG >= 10");
_Static_assert(FLT_DECIMAL_DIG >= 6, "FLT_DECIMAL_DIG >= 6");
_Static_assert(DBL_DECIMAL_DIG >= 10, "DBL_DECIMAL_DIG >= 10");
_Static_assert(LDBL_DECIMAL_DIG >= 10, "LDBL_DECIMAL_DIG >= 10");

/* Mantissa/exponent macros exist and are coherent. */
_Static_assert(FLT_MANT_DIG >= 1 && DBL_MANT_DIG >= FLT_MANT_DIG
            && LDBL_MANT_DIG >= DBL_MANT_DIG,
               "mantissa digits present and non-decreasing across float/double/long double");
