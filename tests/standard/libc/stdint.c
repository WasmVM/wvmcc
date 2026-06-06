/* tests/standard/libc/stdint.c — <stdint.h> limit & constant macros.
 * Catalog: LIBC-stdint-* (docs/standard/libc.md). Verify=static-assert.
 *
 * Scope note: this file covers the integer-constant-MACRO rows only. The
 * type-WIDTH rows (sizeof(int32_t)==4, intptr_t width, INTN_C width, …) need
 * `sizeof`/casts in an ICE, which wvmcc's _Static_assert evaluator does not yet
 * accept — blocked on #81. Those rows land once #81 is fixed. */
#include <stdint.h>

/* LIBC-stdint-INTN_limits-01 — exact-width limit macros */
_Static_assert(INT8_MAX == 127 && UINT8_MAX == 255, "INT8/UINT8 limits");
_Static_assert(INT16_MAX == 32767 && UINT16_MAX == 65535, "INT16/UINT16 limits");
_Static_assert(INT32_MAX == 2147483647 && UINT32_MAX == 4294967295U, "INT32/UINT32 limits");
_Static_assert(INT64_MAX == 9223372036854775807LL && UINT64_MAX == 18446744073709551615ULL,
               "INT64/UINT64 limits");

/* LIBC-stdint-INT_LEAST/FAST-01 — least/fast limit macros (mirror exact here) */
_Static_assert(INT_LEAST32_MAX == 2147483647 && UINT_LEAST32_MAX == 4294967295U, "least32 limits");
_Static_assert(INT_FAST32_MAX == 2147483647 && UINT_FAST32_MAX == 4294967295U, "fast32 limits");

/* LIBC-stdint-other-limits-01 — SIZE_MAX / PTRDIFF_MAX / INTPTR / INTMAX (LP64) */
_Static_assert(SIZE_MAX == 18446744073709551615ULL, "SIZE_MAX (LP64)");
_Static_assert(PTRDIFF_MAX == 9223372036854775807LL, "PTRDIFF_MAX (LP64)");
_Static_assert(INTPTR_MAX == INT64_MAX && INTMAX_MAX == INT64_MAX, "INTPTR/INTMAX max");

/* LIBC-stdint-INTN_C-01 — constant builders yield the right VALUE
 * (width via sizeof is blocked on #81) */
_Static_assert(INT32_C(7) == 7, "INT32_C value");
_Static_assert(INT64_C(1) == 1, "INT64_C value");
_Static_assert(UINT32_C(1) == 1u, "UINT32_C value");
_Static_assert(INTMAX_C(5) == 5 && UINTMAX_C(5) == 5u, "INTMAX_C/UINTMAX_C value");

/* #if-usability of the limit macros.
 * NOTE: uses INT64_MAX (signed) — wvmcc's preprocessor #if evaluator cannot
 * parse full-width unsigned constants like SIZE_MAX/UINT64_MAX
 * (18446744073709551615ULL → "invalid integer constant"). That #if-usability
 * sub-case is tracked separately; the _Static_assert above (semantic ICE
 * evaluator) handles the full-width value fine. */
#if INT32_MAX == 2147483647 && INT64_MAX > INT32_MAX
/* ok */
#else
#error "stdint limit macros must be #if-usable"
#endif
