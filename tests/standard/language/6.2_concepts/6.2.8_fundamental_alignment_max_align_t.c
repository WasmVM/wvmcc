/* LANG-6.2.8-02 — 6.2.8p2: a fundamental alignment is one that is less than or
 * equal to the greatest alignment supported by the implementation in all
 * contexts, i.e. _Alignof(max_align_t). Fundamental alignments are supported for
 * objects of all storage durations. Verify=static-assert (freestanding).
 *
 * Per docs/spec.md max_align_t has alignment 8. The alignment of every basic
 * type must be a fundamental alignment, i.e. <= _Alignof(max_align_t).
 *
 * Compile-only; a held assertion = pass. */
#include <stddef.h>

/* The greatest fundamental alignment (per docs/spec.md). */
_Static_assert(_Alignof(max_align_t) == 8, "_Alignof(max_align_t) == 8");

/* Every basic-type alignment is fundamental: it does not exceed max_align_t. */
_Static_assert(_Alignof(char)   <= _Alignof(max_align_t), "char is fundamental");
_Static_assert(_Alignof(short)  <= _Alignof(max_align_t), "short is fundamental");
_Static_assert(_Alignof(int)    <= _Alignof(max_align_t), "int is fundamental");
_Static_assert(_Alignof(long)   <= _Alignof(max_align_t), "long is fundamental");
_Static_assert(_Alignof(float)  <= _Alignof(max_align_t), "float is fundamental");
_Static_assert(_Alignof(double) <= _Alignof(max_align_t), "double is fundamental");

/* max_align_t is itself at least as aligned as any basic type. */
_Static_assert(_Alignof(max_align_t) >= _Alignof(double),
               "max_align_t aligns >= double");
