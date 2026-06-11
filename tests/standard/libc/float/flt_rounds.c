/* tests/standard/libc/float/flt_rounds.c — FLT_ROUNDS.
 * Catalog: LIBC-float-FLT_ROUNDS-01 (ISO C17 7.7p3, 5.2.4.2.2p8).
 * FLT_ROUNDS characterizes the floating-point addition rounding mode.
 * wvmcc documents round-to-nearest (docs/spec.md) => value 1.
 * Verify=static-assert; compile-only, a held assertion = pass. */
#include <float.h>

/* Standard set of meaningful values is -1..3 (other values are
 * implementation-defined); round-to-nearest is 1. */
_Static_assert(FLT_ROUNDS == 1, "FLT_ROUNDS must be 1 (round to nearest)");
