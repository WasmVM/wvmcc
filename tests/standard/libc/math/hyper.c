/* tests/standard/libc/math/hyper.c — LIBC-math-hyper-01. Verify=exit.
 * C17 7.12.5: hyperbolic functions, checked at exact special points
 * with a loose tolerance. */
#include <math.h>

static int near(double a, double b) {
    double d = a - b;
    if (d < 0.0) d = -d;
    return d < 1e-9;
}

int main(void) {
    if (!near(sinh(0.0), 0.0)) return 1;
    if (!near(cosh(0.0), 1.0)) return 2;
    if (!near(tanh(0.0), 0.0)) return 3;
    if (!near(asinh(0.0), 0.0)) return 4;
    if (!near(acosh(1.0), 0.0)) return 5;
    if (!near(atanh(0.0), 0.0)) return 6;
    if (!near(sinh(1.0), 1.1752011936438014)) return 7;
    if (!near(cosh(1.0), 1.5430806348152437)) return 8;
    return 0;
}
