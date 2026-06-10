/* tests/standard/libc/math/types.c — LIBC-math-types-01. Verify=static-assert.
 * C17 7.12p2: float_t and double_t are the implementation's most-efficient
 * evaluation types. When FLT_EVAL_METHOD == 0 they are float and double. */
#include <math.h>
#include <float.h>

#if FLT_EVAL_METHOD == 0
_Static_assert(_Generic((float_t)0, float: 1, default: 0),
               "FLT_EVAL_METHOD 0: float_t must be float");
_Static_assert(_Generic((double_t)0, double: 1, default: 0),
               "FLT_EVAL_METHOD 0: double_t must be double");
#endif

/* In every evaluation method, float_t/double_t are at least as wide as
 * float/double respectively. */
_Static_assert(sizeof(float_t) >= sizeof(float),
               "float_t at least as wide as float");
_Static_assert(sizeof(double_t) >= sizeof(double),
               "double_t at least as wide as double");
