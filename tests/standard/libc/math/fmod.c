/* tests/standard/libc/math/fmod.c — LIBC-math-fmod-01. Verify=exit.
 * C17 7.12.10: remainder functions. fmod result has the sign of x;
 * remainder is the IEC 60559 remainder (r = x - n*y, n nearest integer);
 * remquo additionally yields at least 3 low-order bits of the quotient,
 * with the sign of x/y. All chosen operands give exact results. */
#include <math.h>
int main(void) {
    if (fmod(5.5, 2.0) != 1.5) return 1;
    if (fmod(-5.5, 2.0) != -1.5) return 2;     /* sign of x */
    if (fmod(6.0, 2.0) != 0.0) return 3;
    if (remainder(5.5, 2.0) != -0.5) return 4; /* n = 3 (nearest), 5.5-6 */
    if (remainder(5.0, 2.0) != 1.0) return 5;  /* n = 2 */
    int q = 0;
    double r = remquo(5.5, 2.0, &q);
    if (r != -0.5) return 6;
    if (q < 0) return 7;                       /* x/y positive */
    if ((q & 7) != 3) return 8;                /* quotient 3, low 3 bits */
    return 0;
}
