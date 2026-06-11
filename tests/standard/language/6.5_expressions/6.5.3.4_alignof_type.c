/* LANG-6.5.3.4-02 — 6.5.3.4p3 (ISO C17): _Alignof(type-name) yields the
 * alignment requirement of its operand type. _Alignof(char) is 1. The
 * alignment of a complete type is a power of two, and a structure's alignment
 * is at least as strict as that of its most-aligned member. The operand of
 * _Alignof is not evaluated.
 * Verify=static-assert (freestanding). A held assertion = pass. */

/* _Alignof(char) is 1. */
_Static_assert(_Alignof(char) == 1, "_Alignof(char) is 1");

/* Every alignment value is a positive power of two. */
_Static_assert(_Alignof(int) > 0 && (_Alignof(int) & (_Alignof(int) - 1)) == 0,
               "_Alignof(int) is a power of two");
_Static_assert(_Alignof(double) > 0 &&
               (_Alignof(double) & (_Alignof(double) - 1)) == 0,
               "_Alignof(double) is a power of two");

/* A struct's alignment is at least that of its most strictly aligned member. */
struct S { char c; double d; };
_Static_assert(_Alignof(struct S) >= _Alignof(double),
               "struct alignment >= member alignment");

/* _Alignof takes a parenthesized type-name and reports that type's alignment;
 * an array type aligns like its element type. */
_Static_assert(_Alignof(int[4]) == _Alignof(int), "array aligns like element");
