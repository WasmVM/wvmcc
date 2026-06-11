/* LANG-5.2.4.2.1-03 — 5.2.4.2.1p2: if an object of type char can hold
 * negative values, CHAR_MIN == SCHAR_MIN and CHAR_MAX == SCHAR_MAX;
 * otherwise CHAR_MIN == 0 and CHAR_MAX == UCHAR_MAX.
 * docs/spec.md documents plain char as signed. */
#include <limits.h>

/* The standard-mandated coupling, whichever signedness is chosen. */
_Static_assert(((char)-1 < 0)
                   ? (CHAR_MIN == SCHAR_MIN && CHAR_MAX == SCHAR_MAX)
                   : (CHAR_MIN == 0 && CHAR_MAX == UCHAR_MAX),
               "CHAR_MIN/CHAR_MAX track char signedness (5.2.4.2.1p2)");

/* docs/spec.md: plain char is signed by default. */
_Static_assert((char)-1 < 0, "plain char is signed (docs/spec.md)");
_Static_assert(CHAR_MIN == SCHAR_MIN, "CHAR_MIN == SCHAR_MIN for signed char");
_Static_assert(CHAR_MAX == SCHAR_MAX, "CHAR_MAX == SCHAR_MAX for signed char");
