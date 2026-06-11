/* tests/standard/libc/math/fmax.c — LIBC-math-fmax-01. Verify=exit.
 * C17 7.12.12: fdim, fmax, fmin. Per F.10.9, fmax/fmin treat a NaN
 * argument as missing data and return the other (numeric) argument. */
#include <math.h>
int main(void) {
    if (fmax(1.0, 2.0) != 2.0) return 1;
    if (fmax(-1.0, -2.0) != -1.0) return 2;
    if (fmin(1.0, 2.0) != 1.0) return 3;
    if (fmin(-1.0, -2.0) != -2.0) return 4;
    if (fdim(3.0, 1.0) != 2.0) return 5;     /* x > y: x - y */
    if (fdim(1.0, 3.0) != 0.0) return 6;     /* x <= y: +0 */
    if (fmax(NAN, 1.0) != 1.0) return 7;     /* NaN treated as missing */
    if (fmin(2.0, NAN) != 2.0) return 8;
    return 0;
}
