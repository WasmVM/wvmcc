/* tests/standard/libc/math/isinf.c — LIBC-math-isinf-01. Verify=exit.
 * C17 7.12.3.3: isinf returns nonzero iff the argument is (positive or
 * negative) infinity. */
#include <math.h>
int main(void) {
    if (!isinf(INFINITY)) return 1;
    if (!isinf(-INFINITY)) return 2;
    if (isinf(0.0)) return 3;
    if (isinf(1.0)) return 4;
    if (isinf(NAN)) return 5;
    return 0;
}
