/* tests/standard/libc/limits.c — <limits.h> constant macros.
 * Catalog: LIBC-limits-* (docs/standard/libc.md). Verify=static-assert.
 * Values follow wvmcc's LP64 model (docs/spec.md): int 32-bit, long 64-bit,
 * signed `char`. Compile-only; a held assertion = pass. */
#include <limits.h>

/* LIBC-limits-CHAR_BIT-01 */
_Static_assert(CHAR_BIT == 8, "CHAR_BIT must be 8");

/* LIBC-limits-SCHAR-01 */
_Static_assert(SCHAR_MIN == -128, "SCHAR_MIN");
_Static_assert(SCHAR_MAX == 127, "SCHAR_MAX");
_Static_assert(UCHAR_MAX == 255, "UCHAR_MAX");

/* LIBC-limits-CHAR-01 — signed `char` default => CHAR_* == SCHAR_* */
_Static_assert(CHAR_MIN == SCHAR_MIN, "CHAR_MIN == SCHAR_MIN (signed char)");
_Static_assert(CHAR_MAX == SCHAR_MAX, "CHAR_MAX == SCHAR_MAX (signed char)");

/* LIBC-limits-SHRT-01 */
_Static_assert(SHRT_MIN == -32768, "SHRT_MIN");
_Static_assert(SHRT_MAX == 32767, "SHRT_MAX");
_Static_assert(USHRT_MAX == 65535, "USHRT_MAX");

/* LIBC-limits-INT-01 — 32-bit */
_Static_assert(INT_MIN == -2147483647 - 1, "INT_MIN");
_Static_assert(INT_MAX == 2147483647, "INT_MAX");
_Static_assert(UINT_MAX == 4294967295U, "UINT_MAX");

/* LIBC-limits-LONG-01 — LP64: long is 64-bit */
_Static_assert(LONG_MAX == 9223372036854775807LL, "LONG_MAX (LP64)");
_Static_assert(ULONG_MAX == 18446744073709551615ULL, "ULONG_MAX (LP64)");

/* LIBC-limits-LLONG-01 */
_Static_assert(LLONG_MAX == 9223372036854775807LL, "LLONG_MAX");
_Static_assert(ULLONG_MAX == 18446744073709551615ULL, "ULLONG_MAX");

/* LIBC-limits-MB_LEN_MAX-01 — standard floor is 1 (wvmcc's header: 4) */
_Static_assert(MB_LEN_MAX >= 1, "MB_LEN_MAX >= 1");

/* LIBC-limits-ICE-01 — usable in #if (the directive elides if non-ICE) */
#if INT_MAX > 0 && LONG_MAX > INT_MAX && CHAR_BIT == 8
/* ok */
#else
#error "limits macros must be #if-usable integer constant expressions"
#endif
