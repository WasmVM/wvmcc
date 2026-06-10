/* tests/standard/libc/stdlib/atoi.c — LIBC-stdlib-atoi-01 (C17 7.22.1.2).
 * atoi / atol / atoll parse int / long / long long. Verify=exit. */
#include <stdlib.h>

int main(void) {
    if (atoi("0") != 0) return 1;
    if (atoi("42") != 42) return 2;
    if (atoi("-7") != -7) return 3;
    if (atoi(" +9") != 9) return 4;  /* white space then optional sign */
    if (atol("-100000") != -100000L) return 5;
    if (atoll("4294967296") != 4294967296LL) return 6;
    return 0;
}
