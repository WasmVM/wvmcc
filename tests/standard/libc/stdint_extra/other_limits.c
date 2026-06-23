/* LIBC-stdint-other-limits-01 — limit macros for the "other" integer types
 * under wvmcc's LP64 model (7.20.3): SIZE_MAX, PTRDIFF_*, INTPTR_*, INTMAX_*,
 * SIG_ATOMIC_*. Verify=static-assert. */
#include <stdint.h>

_Static_assert(SIZE_MAX == 0xFFFFFFFFFFFFFFFFULL, "SIZE_MAX is 64-bit (LP64)");
_Static_assert(PTRDIFF_MAX == 9223372036854775807L, "PTRDIFF_MAX 2^63-1");
_Static_assert(PTRDIFF_MIN == -9223372036854775807L - 1, "PTRDIFF_MIN -2^63");
_Static_assert(INTPTR_MAX == 9223372036854775807L, "INTPTR_MAX 2^63-1");
_Static_assert(INTPTR_MIN == -9223372036854775807L - 1, "INTPTR_MIN -2^63");
_Static_assert(UINTPTR_MAX == 0xFFFFFFFFFFFFFFFFULL, "UINTPTR_MAX 2^64-1");
_Static_assert(INTMAX_MAX == 9223372036854775807L, "INTMAX_MAX 2^63-1");
_Static_assert(UINTMAX_MAX == 0xFFFFFFFFFFFFFFFFULL, "UINTMAX_MAX 2^64-1");

int main(void) { return 0; }
