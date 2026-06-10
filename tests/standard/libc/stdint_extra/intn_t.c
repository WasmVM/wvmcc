/* LIBC-stdint-intN_t-01 — ISO C17 7.20.1.1 Exact-width integer types.
 *
 * intN_t is a signed two's-complement integer type of EXACTLY N bits, no
 * padding; uintN_t the unsigned counterpart. With CHAR_BIT == 8 (verified by
 * LIBC-limits-CHAR_BIT-01) that means sizeof is exactly N/8.
 * Verify = static-assert.
 */
#include <stdint.h>

/* exact width via sizeof */
_Static_assert(sizeof(int8_t)  == 1, "sizeof(int8_t) == 1");
_Static_assert(sizeof(int16_t) == 2, "sizeof(int16_t) == 2");
_Static_assert(sizeof(int32_t) == 4, "sizeof(int32_t) == 4");
_Static_assert(sizeof(int64_t) == 8, "sizeof(int64_t) == 8");

_Static_assert(sizeof(uint8_t)  == 1, "sizeof(uint8_t) == 1");
_Static_assert(sizeof(uint16_t) == 2, "sizeof(uint16_t) == 2");
_Static_assert(sizeof(uint32_t) == 4, "sizeof(uint32_t) == 4");
_Static_assert(sizeof(uint64_t) == 8, "sizeof(uint64_t) == 8");

/* signedness: intN_t signed, uintN_t unsigned */
_Static_assert((int8_t)-1  < 0, "int8_t is signed");
_Static_assert((int16_t)-1 < 0, "int16_t is signed");
_Static_assert((int32_t)-1 < 0, "int32_t is signed");
_Static_assert((int64_t)-1 < 0, "int64_t is signed");

_Static_assert((uint8_t)-1  > 0, "uint8_t is unsigned");
_Static_assert((uint16_t)-1 > 0, "uint16_t is unsigned");
_Static_assert((uint32_t)-1 > 0, "uint32_t is unsigned");
_Static_assert((uint64_t)-1 > 0, "uint64_t is unsigned");

/* two's-complement representation: MIN == -MAX - 1 (7.20.2.1 footnote) */
_Static_assert(INT8_MIN  == -INT8_MAX  - 1, "int8_t two's complement");
_Static_assert(INT16_MIN == -INT16_MAX - 1, "int16_t two's complement");
_Static_assert(INT32_MIN == -INT32_MAX - 1, "int32_t two's complement");
_Static_assert(INT64_MIN == -INT64_MAX - 1, "int64_t two's complement");

int main(void) { return 0; }
