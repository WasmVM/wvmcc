/* LANG-6.2.5-04 — 6.2.5p5: a "plain" int object has the natural size suggested
 * by the architecture (range INT_MIN..INT_MAX), and signed char occupies the
 * same amount of storage as plain char. */
#include <limits.h>

/* signed char and plain char share storage size. */
_Static_assert(sizeof(signed char) == sizeof(char), "signed char == char storage");
_Static_assert(sizeof(signed char) == 1, "signed char is one byte");

/* plain int spans INT_MIN..INT_MAX as defined in <limits.h>. */
_Static_assert(INT_MIN < 0 && INT_MAX > 0, "int range straddles zero");
_Static_assert(INT_MAX == 2147483647, "INT_MAX (32-bit int)");
_Static_assert(INT_MIN == -2147483647 - 1, "INT_MIN (32-bit int)");
