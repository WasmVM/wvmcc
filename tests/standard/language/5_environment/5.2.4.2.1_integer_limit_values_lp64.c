/* LANG-5.2.4.2.1-02 — 5.2.4.2.1: the actual integer limit values are
 * implementation-defined.  docs/spec.md documents the LP64 data model:
 * 8-bit char, 16-bit short, 32-bit int, 64-bit long and long long. */
#include <limits.h>

_Static_assert(CHAR_BIT == 8, "CHAR_BIT == 8");

_Static_assert(SCHAR_MIN == -128, "SCHAR_MIN == -128");
_Static_assert(SCHAR_MAX == 127, "SCHAR_MAX == 127");
_Static_assert(UCHAR_MAX == 255, "UCHAR_MAX == 255");

_Static_assert(SHRT_MIN == -32767 - 1, "SHRT_MIN == -2^15");
_Static_assert(SHRT_MAX == 32767, "SHRT_MAX == 2^15-1");
_Static_assert(USHRT_MAX == 65535, "USHRT_MAX == 2^16-1");

_Static_assert(INT_MIN == -2147483647 - 1, "INT_MIN == -2^31 (int is 32-bit)");
_Static_assert(INT_MAX == 2147483647, "INT_MAX == 2^31-1 (int is 32-bit)");
_Static_assert(UINT_MAX == 4294967295U, "UINT_MAX == 2^32-1");

_Static_assert(LONG_MIN == -9223372036854775807L - 1, "LONG_MIN == -2^63 (long is 64-bit, LP64)");
_Static_assert(LONG_MAX == 9223372036854775807L, "LONG_MAX == 2^63-1 (long is 64-bit, LP64)");
_Static_assert(ULONG_MAX == 18446744073709551615UL, "ULONG_MAX == 2^64-1");

_Static_assert(LLONG_MIN == -9223372036854775807LL - 1, "LLONG_MIN == -2^63");
_Static_assert(LLONG_MAX == 9223372036854775807LL, "LLONG_MAX == 2^63-1");
_Static_assert(ULLONG_MAX == 18446744073709551615ULL, "ULLONG_MAX == 2^64-1");
