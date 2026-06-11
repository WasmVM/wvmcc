/* LIBC-stdint-int_fastN_t-01 — ISO C17 7.20.1.3 Fastest minimum-width
 * integer types.
 *
 * int_fastN_t / uint_fastN_t are "usually fastest" integer types with a width
 * of AT LEAST N bits; the underlying type is implementation-chosen (B-impl),
 * but sizeof >= N/8 is required. Required for N = 8, 16, 32, 64.
 * Verify = static-assert.
 */
#include <stdint.h>

_Static_assert(sizeof(int_fast8_t)  >= 1, "sizeof(int_fast8_t) >= 1");
_Static_assert(sizeof(int_fast16_t) >= 2, "sizeof(int_fast16_t) >= 2");
_Static_assert(sizeof(int_fast32_t) >= 4, "sizeof(int_fast32_t) >= 4");
_Static_assert(sizeof(int_fast64_t) >= 8, "sizeof(int_fast64_t) >= 8");

_Static_assert(sizeof(uint_fast8_t)  >= 1, "sizeof(uint_fast8_t) >= 1");
_Static_assert(sizeof(uint_fast16_t) >= 2, "sizeof(uint_fast16_t) >= 2");
_Static_assert(sizeof(uint_fast32_t) >= 4, "sizeof(uint_fast32_t) >= 4");
_Static_assert(sizeof(uint_fast64_t) >= 8, "sizeof(uint_fast64_t) >= 8");

/* corresponding signed/unsigned pairs have the same size (7.20.1p1) */
_Static_assert(sizeof(int_fast8_t)  == sizeof(uint_fast8_t),  "fast8 pair size");
_Static_assert(sizeof(int_fast16_t) == sizeof(uint_fast16_t), "fast16 pair size");
_Static_assert(sizeof(int_fast32_t) == sizeof(uint_fast32_t), "fast32 pair size");
_Static_assert(sizeof(int_fast64_t) == sizeof(uint_fast64_t), "fast64 pair size");

/* signedness */
_Static_assert((int_fast8_t)-1  < 0, "int_fast8_t is signed");
_Static_assert((int_fast64_t)-1 < 0, "int_fast64_t is signed");
_Static_assert((uint_fast8_t)-1  > 0, "uint_fast8_t is unsigned");
_Static_assert((uint_fast64_t)-1 > 0, "uint_fast64_t is unsigned");

int main(void) { return 0; }
