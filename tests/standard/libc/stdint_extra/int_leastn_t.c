/* LIBC-stdint-int_leastN_t-01 — ISO C17 7.20.1.2 Minimum-width integer types.
 *
 * int_leastN_t / uint_leastN_t have a width of AT LEAST N bits, i.e.
 * sizeof >= N/8 (CHAR_BIT == 8 verified elsewhere). These types are required
 * for N = 8, 16, 32, 64.
 * Verify = static-assert.
 */
#include <stdint.h>

_Static_assert(sizeof(int_least8_t)  >= 1, "sizeof(int_least8_t) >= 1");
_Static_assert(sizeof(int_least16_t) >= 2, "sizeof(int_least16_t) >= 2");
_Static_assert(sizeof(int_least32_t) >= 4, "sizeof(int_least32_t) >= 4");
_Static_assert(sizeof(int_least64_t) >= 8, "sizeof(int_least64_t) >= 8");

_Static_assert(sizeof(uint_least8_t)  >= 1, "sizeof(uint_least8_t) >= 1");
_Static_assert(sizeof(uint_least16_t) >= 2, "sizeof(uint_least16_t) >= 2");
_Static_assert(sizeof(uint_least32_t) >= 4, "sizeof(uint_least32_t) >= 4");
_Static_assert(sizeof(uint_least64_t) >= 8, "sizeof(uint_least64_t) >= 8");

/* corresponding signed/unsigned pairs have the same size (7.20.1p1) */
_Static_assert(sizeof(int_least8_t)  == sizeof(uint_least8_t),  "least8 pair size");
_Static_assert(sizeof(int_least16_t) == sizeof(uint_least16_t), "least16 pair size");
_Static_assert(sizeof(int_least32_t) == sizeof(uint_least32_t), "least32 pair size");
_Static_assert(sizeof(int_least64_t) == sizeof(uint_least64_t), "least64 pair size");

/* signedness */
_Static_assert((int_least8_t)-1  < 0, "int_least8_t is signed");
_Static_assert((int_least64_t)-1 < 0, "int_least64_t is signed");
_Static_assert((uint_least8_t)-1  > 0, "uint_least8_t is unsigned");
_Static_assert((uint_least64_t)-1 > 0, "uint_least64_t is unsigned");

int main(void) { return 0; }
