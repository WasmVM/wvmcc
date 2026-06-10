/* tests/standard/libc/inttypes/imaxabs.c — LIBC-inttypes-imaxabs-01.
 * ISO C17 §7.8.2.1 (imaxabs) and §7.8.2.2 (imaxdiv): greatest-width absolute
 * value and division. imaxdiv computes quot and rem in a single operation;
 * C division truncates toward zero (§6.5.5). Verify=exit. Kept small for the
 * WasmVM interpreter. */
#include <inttypes.h>

int main(void) {
    if (imaxabs((intmax_t)-5) != 5) return 1;
    if (imaxabs((intmax_t)7) != 7) return 2;
    if (imaxabs((intmax_t)0) != 0) return 3;

    imaxdiv_t d = imaxdiv((intmax_t)7, (intmax_t)3);
    if (d.quot != 2) return 4;
    if (d.rem != 1) return 5;

    /* Truncation toward zero: -7/3 == -2 rem -1. */
    d = imaxdiv((intmax_t)-7, (intmax_t)3);
    if (d.quot != -2) return 6;
    if (d.rem != -1) return 7;

    /* Identity: quot*denom + rem == numer. */
    d = imaxdiv((intmax_t)9, (intmax_t)-4);
    if (d.quot * (intmax_t)-4 + d.rem != 9) return 8;

    return 0;
}
