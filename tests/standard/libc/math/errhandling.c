/* tests/standard/libc/math/errhandling.c — LIBC-math-errhandling-01.
 * Verify=static-assert. C17 7.12p9: MATH_ERRNO expands to 1, MATH_ERREXCEPT
 * to 2; math_errhandling is a macro expanding to an int expression (its
 * value need not be an ICE, so only its presence and a usable int context
 * are checked at compile time). */
#include <math.h>

_Static_assert(MATH_ERRNO == 1, "MATH_ERRNO must be 1");
_Static_assert(MATH_ERREXCEPT == 2, "MATH_ERREXCEPT must be 2");

#ifndef math_errhandling
#error "math_errhandling must be defined as a macro by <math.h>"
#endif

/* math_errhandling must be usable as an int expression (value checked at
 * runtime elsewhere; the standard does not require it to be constant). */
int math_errhandling_is_int_expr(void) { return (math_errhandling) | 0; }
