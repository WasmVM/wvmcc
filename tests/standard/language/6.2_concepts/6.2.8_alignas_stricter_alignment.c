/* LANG-6.2.8-03 — 6.2.8p1 (with 6.7.5 _Alignas): an _Alignas specifier may
 * request a stricter (larger) alignment for an object than its type's natural
 * alignment. The resulting object's _Alignof must report the requested value.
 * Verify=static-assert (freestanding).
 *
 * A char naturally has alignment 1; _Alignas(8) makes it alignment 8. _Alignas
 * may also borrow another type's alignment via _Alignas(type-name). It must not
 * weaken an alignment, so the effective alignment is the requested stricter one.
 *
 * Compile-only; a held assertion = pass. */

/* A char over-aligned to 8 reports the requested, stricter alignment. */
_Alignas(8) static char c8;
_Static_assert(_Alignof(c8) == 8, "_Alignas(8) char has alignment 8");

/* _Alignas(type-name) borrows that type's alignment. */
_Alignas(double) static char cd;
_Static_assert(_Alignof(cd) == _Alignof(double),
               "_Alignas(double) char aligns like double");

/* The requested alignment is strictly greater than the natural one here. */
_Static_assert(_Alignof(c8) > _Alignof(char), "_Alignas made it stricter");
