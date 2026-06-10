/* LIBC-stdio-printf-float-01 — C17 7.21.6.1: %f, %e, %g, %a floating
 * conversions, including infinity and NaN. Verify=stdout.
 * Notes on pinned bytes:
 *   - %f default precision is 6; %e exponent has at least two digits.
 *   - %g with value 0.0001 (exponent -4, P=6) uses %f style and strips
 *     trailing zeros -> "0.0001".
 *   - %a of zero must be "0x0p+0" (zero leading digit, zero exponent).
 *   - Infinity/NaN: the standard allows "inf"/"infinity" and
 *     "nan"/"nan(...)"; this fixture pins the shortest conforming form. */
#include <stdio.h>
int main(void) {
    double z = 0.0;
    double one = 1.0;
    printf("%f\n", 1.5);
    printf("%e\n", 1.5);
    printf("%g\n", 0.0001);
    printf("%a\n", z);
    printf("%f %f\n", one / z, z / z);  /* inf nan (Annex F) */
    return 0;
}
