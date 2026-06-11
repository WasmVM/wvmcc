/* tests/standard/libc/math/fp_classes.c — LIBC-math-FP_classes-01.
 * Verify=static-assert. C17 7.12p6: FP_INFINITE, FP_NAN, FP_NORMAL,
 * FP_SUBNORMAL, FP_ZERO are distinct integer constant expressions. */
#include <math.h>

_Static_assert(FP_INFINITE != FP_NAN, "FP_INFINITE != FP_NAN");
_Static_assert(FP_INFINITE != FP_NORMAL, "FP_INFINITE != FP_NORMAL");
_Static_assert(FP_INFINITE != FP_SUBNORMAL, "FP_INFINITE != FP_SUBNORMAL");
_Static_assert(FP_INFINITE != FP_ZERO, "FP_INFINITE != FP_ZERO");
_Static_assert(FP_NAN != FP_NORMAL, "FP_NAN != FP_NORMAL");
_Static_assert(FP_NAN != FP_SUBNORMAL, "FP_NAN != FP_SUBNORMAL");
_Static_assert(FP_NAN != FP_ZERO, "FP_NAN != FP_ZERO");
_Static_assert(FP_NORMAL != FP_SUBNORMAL, "FP_NORMAL != FP_SUBNORMAL");
_Static_assert(FP_NORMAL != FP_ZERO, "FP_NORMAL != FP_ZERO");
_Static_assert(FP_SUBNORMAL != FP_ZERO, "FP_SUBNORMAL != FP_ZERO");
