/* tests/standard/libc/math/isfinite.c — LIBC-math-isfinite-01. Verify=exit.
 * C17 7.12.3.2: isfinite returns nonzero iff the argument is finite
 * (zero, subnormal, or normal; not infinite or NaN). */
#include <math.h>
#include <float.h>
int main(void) {
    if (!isfinite(0.0)) return 1;
    if (!isfinite(1.0)) return 2;
    if (!isfinite(DBL_MIN / 2.0)) return 3;  /* subnormal is finite */
    if (isfinite(INFINITY)) return 4;
    if (isfinite(-INFINITY)) return 5;
    if (isfinite(NAN)) return 6;
    return 0;
}
