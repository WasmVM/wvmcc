/* tests/standard/libc/string/strncat.c — LIBC-string-strncat-01. ISO C17 §7.24.3.2. Verify=exit.
 * strncat appends at most n chars from src, then ALWAYS appends a terminator.
 * Returns dst. */
#include <string.h>
int main(void) {
    /* n smaller than strlen(src): n chars + terminator */
    char dst[8] = {'a', '\0', 'X', 'X', 'X', 'X', 'X', 'X'};
    if (strncat(dst, "bcde", 2) != dst) return 1;
    if (dst[0] != 'a' || dst[1] != 'b' || dst[2] != 'c') return 2;
    if (dst[3] != '\0') return 3; /* terminator always appended */
    if (dst[4] != 'X') return 4;
    /* n larger than strlen(src): stops at src's terminator */
    char d2[6] = {'x', '\0', 'X', 'X', 'X', 'X'};
    strncat(d2, "y", 5);
    if (d2[0] != 'x' || d2[1] != 'y' || d2[2] != '\0') return 5;
    if (d2[3] != 'X') return 6;
    return 0;
}
