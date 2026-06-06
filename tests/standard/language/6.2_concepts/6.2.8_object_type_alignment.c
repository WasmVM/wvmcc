/* LANG-6.2.8-01 — 6.2.8p1: each complete object type has an
 * implementation-defined alignment, a power-of-two value that is the number of
 * bytes between successive addresses at which a given object may be allocated.
 * Verify=static-assert (freestanding).
 *
 * Per docs/spec.md the fundamental alignments are: char 1, short 2, int/float 4,
 * long/double 8. _Alignof on these complete object types must report exactly
 * those values, and every alignment must be a positive power of two.
 *
 * Compile-only; a held assertion = pass. */

/* Each fundamental type's alignment matches the documented value. */
_Static_assert(_Alignof(char)   == 1, "_Alignof(char) == 1");
_Static_assert(_Alignof(short)  == 2, "_Alignof(short) == 2");
_Static_assert(_Alignof(int)    == 4, "_Alignof(int) == 4");
_Static_assert(_Alignof(float)  == 4, "_Alignof(float) == 4");
_Static_assert(_Alignof(long)   == 8, "_Alignof(long) == 8");
_Static_assert(_Alignof(double) == 8, "_Alignof(double) == 8");

/* Every object-type alignment is a positive power of two (6.2.8p1, footnote). */
_Static_assert(_Alignof(int) > 0 && (_Alignof(int) & (_Alignof(int) - 1)) == 0,
               "alignment is a positive power of two");
_Static_assert(_Alignof(double) > 0
                   && (_Alignof(double) & (_Alignof(double) - 1)) == 0,
               "alignment is a positive power of two");
