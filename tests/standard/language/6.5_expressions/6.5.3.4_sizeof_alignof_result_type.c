/* LANG-6.5.3.4-05 — 6.5.3.4p5 (ISO C17): the value of the result of both
 * sizeof and _Alignof is implementation-defined, and its type (an unsigned
 * integer type) is size_t. On this LP64 implementation size_t is a 64-bit
 * unsigned type (see docs/spec.md). The result type of sizeof/_Alignof must be
 * the same unsigned type for both, and it must be unsigned.
 * Verify=static-assert (freestanding). A held assertion = pass. */

/* The result is unsigned: -1 in the result type wraps to a large value, so it
 * is greater than zero. Subtracting more than the value also stays unsigned. */
_Static_assert(sizeof(int) - sizeof(int) - 1 > 0,
               "sizeof result type is unsigned");
_Static_assert(_Alignof(int) - _Alignof(int) - 1 > 0,
               "_Alignof result type is unsigned");

/* The result type of sizeof is size_t, so sizeof(sizeof(x)) is the size of
 * size_t itself. On this LP64 implementation size_t is a 64-bit type. */
_Static_assert(sizeof(sizeof(int)) == 8, "sizeof yields a 64-bit size_t (LP64)");

/* sizeof and _Alignof share the same result type (size_t), so the size of each
 * result is identical. */
_Static_assert(sizeof(sizeof(int)) == sizeof(_Alignof(int)),
               "sizeof and _Alignof have the same result type");
