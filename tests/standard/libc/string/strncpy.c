/* tests/standard/libc/string/strncpy.c — LIBC-string-strncpy-01. ISO C17 §7.24.2.4. Verify=exit.
 * strncpy copies at most n chars; if src is shorter, null-pads dst to n chars.
 * If src is >= n chars, the result is NOT null-terminated. Returns dst. */
#include <string.h>
int main(void) {
    /* short src: copy + null-pad to n */
    char dst[5] = {'X', 'X', 'X', 'X', 'X'};
    if (strncpy(dst, "ab", 4) != dst) return 1;
    if (dst[0] != 'a' || dst[1] != 'b') return 2;
    if (dst[2] != '\0' || dst[3] != '\0') return 3; /* null padding */
    if (dst[4] != 'X') return 4;                    /* beyond n untouched */
    /* long src: exactly n chars, no terminator */
    char d2[4] = {'X', 'X', 'X', 'X'};
    strncpy(d2, "abcdef", 3);
    if (d2[0] != 'a' || d2[1] != 'b' || d2[2] != 'c') return 5;
    if (d2[3] != 'X') return 6; /* no terminator written */
    return 0;
}
