/* LIBC-stdint-intptr_t-01 — ISO C17 7.20.1.4 Integer types capable of
 * holding object pointers.
 *
 * intptr_t / uintptr_t: any valid void* can be converted to these types and
 * back, comparing equal to the original. A necessary compile-time consequence
 * is sizeof(intptr_t) >= sizeof(void *). The width itself is B-impl; wvmcc
 * documents LP64 (docs/spec.md): 64-bit.
 * Verify = static-assert (round-trip itself is checked at runtime in main).
 */
#include <stdint.h>

/* capacity: must be able to hold a void* losslessly */
_Static_assert(sizeof(intptr_t)  >= sizeof(void *), "intptr_t holds void*");
_Static_assert(sizeof(uintptr_t) >= sizeof(void *), "uintptr_t holds void*");

/* signed/unsigned pair, same size (7.20.1p1) */
_Static_assert(sizeof(intptr_t) == sizeof(uintptr_t), "intptr/uintptr pair size");
_Static_assert((intptr_t)-1 < 0,  "intptr_t is signed");
_Static_assert((uintptr_t)-1 > 0, "uintptr_t is unsigned");

/* documented LP64 data model: pointers and intptr_t are 64-bit */
_Static_assert(sizeof(intptr_t) == 8, "intptr_t is 64-bit (LP64)");

/* limit-macro consistency (7.20.2.4) */
_Static_assert(INTPTR_MIN == -INTPTR_MAX - 1, "intptr_t two's complement");

int main(void)
{
    /* 7.20.1.4: void* -> (u)intptr_t -> void* round-trips */
    int obj = 42;
    void *p = &obj;
    if ((void *)(intptr_t)p != p)
        return 1;
    if ((void *)(uintptr_t)p != p)
        return 2;
    return 0;
}
