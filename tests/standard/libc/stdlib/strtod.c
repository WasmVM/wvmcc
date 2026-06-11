/* tests/standard/libc/stdlib/strtod.c — LIBC-stdlib-strtod-01 (C17 7.22.1.3).
 * strtod / strtof / strtold parse floating values with an end pointer;
 * overflow sets errno to ERANGE. Verify=exit. */
#include <stdlib.h>
#include <errno.h>

int main(void) {
    char *end = 0;
    if (strtod("2.5x", &end) != 2.5) return 1;
    if (end == 0 || *end != 'x') return 2;   /* end points at first unconsumed char */
    if (strtod("-0.5", 0) != -0.5) return 3;
    if (strtof("1.5", 0) != 1.5f) return 4;
    if (strtold("0.25", 0) != 0.25L) return 5;
    errno = 0;
    (void)strtod("1e9999", 0);               /* overflow -> +/-HUGE_VAL */
    if (errno != ERANGE) return 6;
    return 0;
}
