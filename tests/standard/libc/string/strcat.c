/* tests/standard/libc/string/strcat.c — LIBC-string-strcat-01. ISO C17 §7.24.3.1. Verify=exit.
 * strcat appends src (including its terminator) to the end of dst; returns dst. */
#include <string.h>
int main(void) {
    char dst[8] = {'a', 'b', '\0', 'X', 'X', 'X', 'X', 'X'};
    if (strcat(dst, "cd") != dst) return 1;
    if (dst[0] != 'a' || dst[1] != 'b' || dst[2] != 'c' || dst[3] != 'd') return 2;
    if (dst[4] != '\0') return 3; /* terminator appended */
    if (dst[5] != 'X') return 4;
    /* appending "" leaves dst unchanged */
    strcat(dst, "");
    if (dst[4] != '\0' || dst[3] != 'd') return 5;
    return 0;
}
