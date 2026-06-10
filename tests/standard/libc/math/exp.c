/* tests/standard/libc/math/exp.c — LIBC-math-exp-01. Verify=exit.
 * C17 7.12.6: exponential and logarithmic functions. Exactly-representable
 * results (frexp, ldexp, modf, scalbn, ilogb, logb, powers of two) are
 * compared exactly; transcendental values use a loose tolerance. */
#include <math.h>

static int near(double a, double b) {
    double d = a - b;
    if (d < 0.0) d = -d;
    return d < 1e-9;
}

int main(void) {
    if (!near(exp(0.0), 1.0)) return 1;
    if (!near(exp(1.0), 2.718281828459045)) return 2;
    if (exp2(3.0) != 8.0) return 3;            /* exact in IEC 60559 */
    if (!near(expm1(0.0), 0.0)) return 4;
    if (!near(log(1.0), 0.0)) return 5;
    if (!near(log2(8.0), 3.0)) return 6;
    if (!near(log10(100.0), 2.0)) return 7;
    if (!near(log1p(0.0), 0.0)) return 8;

    int e = 0;
    double frac = frexp(8.0, &e);              /* 8 = 0.5 * 2^4 */
    if (frac != 0.5 || e != 4) return 9;
    if (ldexp(1.5, 3) != 12.0) return 10;
    if (ilogb(8.0) != 3) return 11;
    if (logb(8.0) != 3.0) return 12;

    double ip = 0.0;
    double fp = modf(2.5, &ip);                /* 2.5 = 2 + 0.5, both exact */
    if (fp != 0.5 || ip != 2.0) return 13;
    if (scalbn(1.0, 3) != 8.0) return 14;
    if (scalbln(1.0, 3L) != 8.0) return 15;
    return 0;
}
