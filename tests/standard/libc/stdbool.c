/* tests/standard/libc/stdbool.c — <stdbool.h> macros.
 * Catalog: LIBC-stdbool-* (docs/standard/libc.md). Verify=static-assert.
 *
 * Scope note: sizeof(bool)==1 and the (bool)cast normalization rows need
 * sizeof/casts in an ICE (blocked on #81); they land once #81 is fixed. */
#include <stdbool.h>

/* LIBC-stdbool-true-01 */
_Static_assert(true == 1, "true == 1");
_Static_assert(false == 0, "false == 0");

/* LIBC-stdbool-defined-01 */
_Static_assert(__bool_true_false_are_defined == 1, "__bool_true_false_are_defined == 1");
