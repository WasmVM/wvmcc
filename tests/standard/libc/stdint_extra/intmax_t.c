/* LIBC-stdint-intmax_t-01 — ISO C17 7.20.1.5 Greatest-width integer types.
 *
 * intmax_t can represent any value of any signed integer type; uintmax_t any
 * value of any unsigned integer type. Hence sizeof(intmax_t) >= sizeof(long
 * long) (and every other standard integer type). Width is B-impl; wvmcc
 * documents 64-bit (LP64, docs/spec.md).
 * Verify = static-assert.
 */
#include <stdint.h>

/* must be at least as wide as every standard integer type */
_Static_assert(sizeof(intmax_t) >= sizeof(long long), "intmax_t >= long long");
_Static_assert(sizeof(intmax_t) >= sizeof(long),      "intmax_t >= long");
_Static_assert(sizeof(intmax_t) >= sizeof(int),       "intmax_t >= int");
_Static_assert(sizeof(uintmax_t) >= sizeof(unsigned long long),
               "uintmax_t >= unsigned long long");

/* signed/unsigned pair, same size (7.20.1p1) */
_Static_assert(sizeof(intmax_t) == sizeof(uintmax_t), "intmax/uintmax pair size");
_Static_assert((intmax_t)-1 < 0,  "intmax_t is signed");
_Static_assert((uintmax_t)-1 > 0, "uintmax_t is unsigned");

/* documented choice: 64-bit greatest-width types */
_Static_assert(sizeof(intmax_t) == 8, "intmax_t is 64-bit");

/* limit-macro consistency (7.20.2.5): INTMAX_MAX covers long long's range */
_Static_assert(INTMAX_MAX >= 9223372036854775807LL, "INTMAX_MAX >= LLONG_MAX value");
_Static_assert(INTMAX_MIN == -INTMAX_MAX - 1, "intmax_t two's complement");

int main(void) { return 0; }
