/* LANG-5.2.4.2.1-01 — 5.2.4.2.1p1: the <limits.h> macros expand to integer
 * constant expressions suitable for use in #if directives, with magnitudes
 * (absolute values) at least the standard minimums and the same sign. */
#include <limits.h>

/* #if-usability: each macro must be usable in conditional inclusion. */
#if CHAR_BIT < 8
#error "CHAR_BIT below minimum"
#endif
#if SCHAR_MAX < 127 || SCHAR_MIN > -127
#error "signed char range below minimum"
#endif
#if UCHAR_MAX < 255
#error "UCHAR_MAX below minimum"
#endif
#if SHRT_MAX < 32767 || SHRT_MIN > -32767 || USHRT_MAX < 65535
#error "short range below minimum"
#endif
#if INT_MAX < 32767 || INT_MIN > -32767 || UINT_MAX < 65535
#error "int range below minimum"
#endif
#if LONG_MAX < 2147483647L || LONG_MIN > -2147483647L || ULONG_MAX < 4294967295UL
#error "long range below minimum"
#endif
#if LLONG_MAX < 9223372036854775807LL || LLONG_MIN > -9223372036854775807LL
#error "long long range below minimum"
#endif
#if ULLONG_MAX < 18446744073709551615ULL
#error "ULLONG_MAX below minimum"
#endif
#if MB_LEN_MAX < 1
#error "MB_LEN_MAX below minimum"
#endif

/* Same checks as integer constant expressions. */
_Static_assert(CHAR_BIT >= 8, "CHAR_BIT >= 8");
_Static_assert(SCHAR_MAX >= 127, "SCHAR_MAX >= 127");
_Static_assert(SCHAR_MIN <= -127, "SCHAR_MIN <= -127");
_Static_assert(UCHAR_MAX >= 255, "UCHAR_MAX >= 255");
_Static_assert(CHAR_MAX >= 127 || CHAR_MAX >= 255, "CHAR_MAX covers its signedness minimum");
_Static_assert(SHRT_MAX >= 32767, "SHRT_MAX >= 32767");
_Static_assert(SHRT_MIN <= -32767, "SHRT_MIN <= -32767");
_Static_assert(USHRT_MAX >= 65535, "USHRT_MAX >= 65535");
_Static_assert(INT_MAX >= 32767, "INT_MAX >= 32767");
_Static_assert(INT_MIN <= -32767, "INT_MIN <= -32767");
_Static_assert(UINT_MAX >= 65535, "UINT_MAX >= 65535");
_Static_assert(LONG_MAX >= 2147483647L, "LONG_MAX >= 2^31-1");
_Static_assert(LONG_MIN <= -2147483647L, "LONG_MIN <= -(2^31-1)");
_Static_assert(ULONG_MAX >= 4294967295UL, "ULONG_MAX >= 2^32-1");
_Static_assert(LLONG_MAX >= 9223372036854775807LL, "LLONG_MAX >= 2^63-1");
_Static_assert(LLONG_MIN <= -9223372036854775807LL, "LLONG_MIN <= -(2^63-1)");
_Static_assert(ULLONG_MAX >= 18446744073709551615ULL, "ULLONG_MAX >= 2^64-1");
_Static_assert(MB_LEN_MAX >= 1, "MB_LEN_MAX >= 1");

/* Correct sign: maxima positive, minima negative. */
_Static_assert(SCHAR_MIN < 0 && SHRT_MIN < 0 && INT_MIN < 0 && LONG_MIN < 0
            && LLONG_MIN < 0, "minima are negative");
_Static_assert(SCHAR_MAX > 0 && SHRT_MAX > 0 && INT_MAX > 0 && LONG_MAX > 0
            && LLONG_MAX > 0, "maxima are positive");
