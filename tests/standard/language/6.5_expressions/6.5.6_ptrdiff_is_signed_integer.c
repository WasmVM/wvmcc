/* LANG-6.5.6-13 — 6.5.6p9 (ISO C17): the type of the result of subtracting two
 * pointers is ptrdiff_t, which is a signed integer type (7.19). On this target
 * docs/spec.md fixes ptrdiff_t as signed 64-bit (i64). Verify=static-assert
 * (freestanding); a held assertion = pass. */
#include <stddef.h>

/* ptrdiff_t is an integer type. */
_Static_assert((ptrdiff_t)1 / (ptrdiff_t)2 == 0, "ptrdiff_t has integer division");

/* It is a signed type: -1 < 0 in ptrdiff_t (an unsigned type would make it large). */
_Static_assert((ptrdiff_t)-1 < (ptrdiff_t)0, "ptrdiff_t is signed");

/* Its width matches the subtraction-result type, here 64-bit on this target. */
_Static_assert(sizeof(ptrdiff_t) == 8, "ptrdiff_t is 64-bit (i64) on this target");

/* The result of ptr - ptr actually has type ptrdiff_t. */
static int arr[4];
_Static_assert(_Generic(&arr[3] - &arr[0], ptrdiff_t: 1, default: 0),
               "ptr - ptr has type ptrdiff_t");
