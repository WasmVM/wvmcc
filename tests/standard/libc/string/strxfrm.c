/* tests/standard/libc/string/strxfrm.c — LIBC-string-strxfrm-01. ISO C17 §7.24.4.5. Verify=exit.
 * strxfrm transforms src so that strcmp on transformed strings orders like
 * strcoll on the originals. Returns the length of the transformed string;
 * if the return is < n the result is a usable null-terminated string. */
#include <string.h>
int main(void) {
    char ta[8], tb[8];
    size_t la = strxfrm(ta, "ab", 8);
    size_t lb = strxfrm(tb, "ac", 8);
    if (la >= 8 || lb >= 8) return 1;        /* fits: results are valid strings */
    if (strlen(ta) != la) return 2;          /* return == length of result */
    int x = strcmp(ta, tb);
    int c = strcoll("ab", "ac");
    if (!((x < 0 && c < 0) || (x == 0 && c == 0) || (x > 0 && c > 0))) return 3;
    /* n == 0 with null dst permitted: returns required length */
    if (strxfrm(NULL, "ab", 0) != la) return 4;
    return 0;
}
