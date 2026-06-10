/* tests/standard/libc/string/strcpy.c — LIBC-string-strcpy-01. ISO C17 §7.24.2.3. Verify=exit.
 * strcpy copies the string including its null terminator and returns dst. */
#include <string.h>
int main(void) {
    char dst[6] = {'X', 'X', 'X', 'X', 'X', 'X'};
    if (strcpy(dst, "abc") != dst) return 1;
    if (dst[0] != 'a' || dst[1] != 'b' || dst[2] != 'c') return 2;
    if (dst[3] != '\0') return 3; /* terminator copied */
    if (dst[4] != 'X') return 4;  /* bytes past the terminator untouched */
    char e[2] = {'X', 'X'};
    strcpy(e, "");
    if (e[0] != '\0' || e[1] != 'X') return 5;
    return 0;
}
