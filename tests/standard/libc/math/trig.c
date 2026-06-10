/* tests/standard/libc/math/trig.c — LIBC-math-trig-01. Verify=exit.
 * C17 7.12.4: trigonometric functions. Accuracy is implementation-defined
 * (7.12p1 / F.10), so values are checked against a loose tolerance. */
#include <math.h>

static int near(double a, double b) {
    double d = a - b;
    if (d < 0.0) d = -d;
    return d < 1e-9;
}

int main(void) {
    if (!near(sin(0.0), 0.0)) return 1;
    if (!near(cos(0.0), 1.0)) return 2;
    if (!near(tan(0.0), 0.0)) return 3;
    if (!near(asin(1.0), 1.5707963267948966)) return 4;   /* pi/2 */
    if (!near(acos(1.0), 0.0)) return 5;
    if (!near(atan(1.0), 0.7853981633974483)) return 6;   /* pi/4 */
    if (!near(atan2(1.0, 1.0), 0.7853981633974483)) return 7;
    if (!near(atan2(0.0, -1.0), 3.141592653589793)) return 8;  /* pi */
    return 0;
}
