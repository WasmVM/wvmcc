/* tests/standard/libc/math/pow.c — LIBC-math-pow-01. Verify=exit.
 * C17 7.12.7: power and absolute-value functions, at exactly-representable
 * points. */
#include <math.h>
int main(void) {
    if (pow(2.0, 10.0) != 1024.0) return 1;
    if (pow(2.0, 0.0) != 1.0) return 2;       /* pow(x,0) == 1 (F.10.4.4) */
    if (sqrt(9.0) != 3.0) return 3;           /* sqrt exactly rounded */
    if (sqrt(0.0) != 0.0) return 4;
    if (cbrt(27.0) != 3.0) return 5;
    if (hypot(3.0, 4.0) != 5.0) return 6;
    if (fabs(-2.5) != 2.5) return 7;
    if (fabs(2.5) != 2.5) return 8;
    if (signbit(fabs(-0.0))) return 9;        /* fabs clears the sign bit */
    return 0;
}
