/* tests/standard/libc/stdlib/atof.c — LIBC-stdlib-atof-01 (C17 7.22.1.1).
 * atof(s) parses a double, equivalent to strtod(s, NULL). Verify=exit. */
#include <stdlib.h>

int main(void) {
    if (atof("0") != 0.0) return 1;
    if (atof("2.5") != 2.5) return 2;
    if (atof("-1.25") != -1.25) return 3;
    if (atof(" 3") != 3.0) return 4; /* leading white space is skipped */
    return 0;
}
