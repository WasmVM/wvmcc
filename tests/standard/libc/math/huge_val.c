/* tests/standard/libc/math/huge_val.c — LIBC-math-HUGE_VAL-01. Verify=exit.
 * C17 7.12p3-5: HUGE_VAL/HUGE_VALF/HUGE_VALL are positive; with IEC 60559
 * arithmetic INFINITY is positive infinity and NAN is a quiet NaN. */
#include <math.h>
int main(void) {
    double inf = INFINITY;
    if (!(inf > 1e308)) return 1;          /* exceeds DBL_MAX => infinite */
    float qn = NAN;
    if (qn == qn) return 2;                /* NaN compares unequal to itself */
    if (!(HUGE_VAL > 0.0)) return 3;
    if (!(HUGE_VALF > 0.0f)) return 4;
    if (!(HUGE_VALL > 0.0L)) return 5;
    return 0;
}
