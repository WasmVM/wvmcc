/* tests/standard/libc/stdbool_extra/bool_macro.c — LIBC-stdbool-bool-01.
 * C17 7.18p1: <stdbool.h> defines the object-like macro `bool`, which
 * expands to `_Bool`. Verify=static-assert (freestanding).
 *
 * Tests the ISO-correct behavior, including properties that require
 * casts inside an integer constant expression (C17 6.6p6 permits casts
 * to integer types, and floating constants as immediate cast operands).
 */
#include <stdbool.h>

/* `bool` names the type _Bool (checked via _Generic type selection). */
_Static_assert(_Generic((bool)0, _Bool: 1, default: 0) == 1,
               "bool expands to _Bool");

/* Conversion to _Bool normalizes to 0/1 (C17 6.3.1.2p1). */
_Static_assert((bool)0 == 0, "(bool)0 == 0");
_Static_assert((bool)1 == 1, "(bool)1 == 1");
_Static_assert((bool)2 == 1, "(bool)2 normalizes to 1");
_Static_assert((bool)0.5 == 1, "(bool)0.5 normalizes to 1");

/* _Bool occupies at least one byte; sizeof is an ICE (C17 6.5.3.4). */
_Static_assert(sizeof(bool) >= 1, "sizeof(bool) >= 1");
_Static_assert(sizeof(bool) == sizeof(_Bool), "sizeof(bool) == sizeof(_Bool)");
