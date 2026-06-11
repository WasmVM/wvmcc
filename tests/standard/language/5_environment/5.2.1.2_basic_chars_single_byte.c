/* LANG-5.2.1.2-01 — 5.2.1.2p1: each member of the basic character set is
 * encoded in a single byte (and per 6.2.5p3 fits in a char as a nonnegative
 * value); a byte with all bits zero is never part of any multibyte character;
 * MB_LEN_MAX (<limits.h>) bounds the bytes in a multibyte character. */
#include <limits.h>

_Static_assert(MB_LEN_MAX >= 1, "MB_LEN_MAX is at least 1");

/* Basic-set members fit in a single char object with nonnegative value. */
_Static_assert((char)'A' == 'A' && 'A' > 0, "'A' fits in one byte, nonnegative");
_Static_assert((char)'z' == 'z' && 'z' > 0, "'z' fits in one byte, nonnegative");
_Static_assert((char)'9' == '9' && '9' > 0, "'9' fits in one byte, nonnegative");
_Static_assert((char)' ' == ' ' && ' ' > 0, "space fits in one byte, nonnegative");
_Static_assert((char)'~' == '~' && '~' > 0, "'~' fits in one byte, nonnegative");
_Static_assert('A' <= UCHAR_MAX, "basic-set member value is within one byte");
