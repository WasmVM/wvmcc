/* tests/standard/libc/math/isnormal.c — LIBC-math-isnormal-01. Verify=exit.
 * C17 7.12.3.5: isnormal returns nonzero iff the argument is normal
 * (not zero, subnormal, infinite, or NaN). */
#include <math.h>
#include <float.h>
int main(void) {
    if (!isnormal(1.0)) return 1;
    if (!isnormal(-2.5)) return 2;
    if (isnormal(0.0)) return 3;
    if (isnormal(DBL_MIN / 2.0)) return 4;  /* subnormal is not normal */
    if (isnormal(INFINITY)) return 5;
    if (isnormal(NAN)) return 6;
    return 0;
}
