/* tests/standard/libc/stdlib/div.c — LIBC-stdlib-div-01 (C17 7.22.6.2).
 * div / ldiv / lldiv compute quotient and remainder in a single operation;
 * quot is truncated toward zero and quot*denom + rem == numer. Verify=exit. */
#include <stdlib.h>

int main(void) {
    div_t d = div(7, 2);
    if (d.quot != 3 || d.rem != 1) return 1;
    d = div(-7, 2);                       /* truncation toward zero */
    if (d.quot != -3 || d.rem != -1) return 2;
    ldiv_t ld = ldiv(7L, -2L);
    if (ld.quot != -3L || ld.rem != 1L) return 3;
    lldiv_t lld = lldiv(9LL, 4LL);
    if (lld.quot != 2LL || lld.rem != 1LL) return 4;
    return 0;
}
