/* tests/standard/libc/math/isnan.c — LIBC-math-isnan-01. Verify=exit.
 * C17 7.12.3.4: isnan returns nonzero iff the argument is a NaN. */
#include <math.h>
int main(void) {
    if (!isnan(NAN)) return 1;
    if (isnan(0.0)) return 2;
    if (isnan(1.0)) return 3;
    if (isnan(INFINITY)) return 4;
    if (!isnan((float)NAN)) return 5;   /* float argument */
    return 0;
}
