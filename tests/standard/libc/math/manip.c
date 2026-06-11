/* tests/standard/libc/math/manip.c — LIBC-math-manip-01. Verify=exit.
 * C17 7.12.11: manipulation functions — copysign, nan, nextafter,
 * nexttoward. nextafter(1,2) is exactly 1 + DBL_EPSILON. */
#include <math.h>
#include <float.h>
int main(void) {
    if (copysign(3.0, -1.0) != -3.0) return 1;
    if (copysign(-3.0, 1.0) != 3.0) return 2;
    if (!signbit(copysign(0.0, -1.0))) return 3;  /* sign applies to zero */

    double n = nan("");
    if (!isnan(n)) return 4;                       /* nan("") is a quiet NaN */

    double up = nextafter(1.0, 2.0);
    if (!(up > 1.0)) return 5;
    if (up != 1.0 + DBL_EPSILON) return 6;         /* next representable */
    double dn = nextafter(1.0, 0.0);
    if (!(dn < 1.0)) return 7;
    if (nextafter(1.0, 1.0) != 1.0) return 8;      /* x == y => y */

    if (nexttoward(1.0, 2.0L) != up) return 9;
    return 0;
}
