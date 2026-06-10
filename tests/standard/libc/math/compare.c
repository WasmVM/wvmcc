/* tests/standard/libc/math/compare.c — LIBC-math-compare-01. Verify=exit.
 * C17 7.12.14: quiet comparison macros — like the relational operators but
 * never raise "invalid" on NaN; all relations except isunordered are false
 * when either argument is NaN. */
#include <math.h>
int main(void) {
    if (!isgreater(2.0, 1.0)) return 1;
    if (isgreater(1.0, 2.0)) return 2;
    if (isgreater(NAN, 1.0)) return 3;            /* NaN: false */
    if (!isgreaterequal(2.0, 2.0)) return 4;
    if (!isless(1.0, 2.0)) return 5;
    if (isless(NAN, 1.0)) return 6;
    if (!islessequal(1.0, 1.0)) return 7;
    if (!islessgreater(1.0, 2.0)) return 8;
    if (islessgreater(1.0, 1.0)) return 9;
    if (islessgreater(NAN, 1.0)) return 10;
    if (!isunordered(NAN, 1.0)) return 11;
    if (isunordered(1.0, 2.0)) return 12;
    return 0;
}
