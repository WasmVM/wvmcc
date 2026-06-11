/* tests/standard/libc/stdlib/strtol.c — LIBC-stdlib-strtol-01 (C17 7.22.1.4).
 * strtol / strtoll / strtoul / strtoull parse integers with base and end
 * pointer; out-of-range values clamp and set errno to ERANGE. Verify=exit. */
#include <stdlib.h>
#include <errno.h>
#include <limits.h>

int main(void) {
    char *end = 0;
    if (strtol("123x", &end, 10) != 123L) return 1;
    if (end == 0 || *end != 'x') return 2;
    if (strtol("ff", 0, 16) != 255L) return 3;
    if (strtol("0x10", 0, 0) != 16L) return 4;  /* base 0: hex prefix */
    if (strtol("010", 0, 0) != 8L) return 5;    /* base 0: octal prefix */
    if (strtoul("4294967295", 0, 10) != 4294967295UL) return 6;
    if (strtoll("-5", 0, 10) != -5LL) return 7;
    if (strtoull("10", 0, 16) != 16ULL) return 8;
    errno = 0;
    if (strtol("99999999999999999999", 0, 10) != LONG_MAX) return 9; /* clamp */
    if (errno != ERANGE) return 10;
    return 0;
}
