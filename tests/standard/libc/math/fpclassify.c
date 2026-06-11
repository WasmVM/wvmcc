/* tests/standard/libc/math/fpclassify.c — LIBC-math-fpclassify-01. Verify=exit.
 * C17 7.12.3.1: fpclassify classifies its argument as NaN, infinite, normal,
 * subnormal, or zero. */
#include <math.h>
#include <float.h>
int main(void) {
    if (fpclassify(0.0) != FP_ZERO) return 1;
    if (fpclassify(-0.0) != FP_ZERO) return 2;
    if (fpclassify(1.0) != FP_NORMAL) return 3;
    if (fpclassify(INFINITY) != FP_INFINITE) return 4;
    if (fpclassify(NAN) != FP_NAN) return 5;
    double sub = DBL_MIN / 2.0;        /* subnormal under IEC 60559 */
    if (fpclassify(sub) != FP_SUBNORMAL) return 6;
    if (fpclassify(1.0f) != FP_NORMAL) return 7;  /* float argument */
    return 0;
}
