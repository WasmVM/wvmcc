/* tests/standard/libc/math/round.c — LIBC-math-round-01. Verify=exit.
 * C17 7.12.9: nearest-integer functions. round() rounds halfway cases away
 * from zero regardless of rounding direction; rint/nearbyint/lrint use the
 * current (default to-nearest) mode, so only unambiguous points are used. */
#include <math.h>
int main(void) {
    if (ceil(1.2) != 2.0) return 1;
    if (ceil(-1.2) != -1.0) return 2;
    if (floor(1.8) != 1.0) return 3;
    if (floor(-1.8) != -2.0) return 4;
    if (trunc(-1.7) != -1.0) return 5;
    if (trunc(1.7) != 1.0) return 6;
    if (round(2.5) != 3.0) return 7;          /* halfway: away from zero */
    if (round(-2.5) != -3.0) return 8;
    if (lround(2.5) != 3L) return 9;
    if (llround(-2.5) != -3LL) return 10;
    if (nearbyint(2.0) != 2.0) return 11;
    if (rint(2.0) != 2.0) return 12;
    if (lrint(3.0) != 3L) return 13;
    if (llrint(3.0) != 3LL) return 14;
    return 0;
}
