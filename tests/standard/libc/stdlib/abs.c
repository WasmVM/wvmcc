/* tests/standard/libc/stdlib/abs.c — LIBC-stdlib-abs-01 (C17 7.22.6.1).
 * abs / labs / llabs compute the integer absolute value. Verify=exit. */
#include <stdlib.h>

int main(void) {
    if (abs(-5) != 5) return 1;
    if (abs(5) != 5) return 2;
    if (abs(0) != 0) return 3;
    if (labs(-70000L) != 70000L) return 4;
    if (llabs(-5000000000LL) != 5000000000LL) return 5;
    return 0;
}
