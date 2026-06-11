/* tests/standard/libc/string/memcmp.c — LIBC-string-memcmp-01. ISO C17 §7.24.4.1. Verify=exit.
 * memcmp compares n bytes as unsigned char; sign follows the first differing pair.
 * Checks sign only, not magnitude. */
#include <string.h>
int main(void) {
    if (memcmp("abc", "abc", 3) != 0) return 1;
    if (!(memcmp("abc", "abd", 3) < 0)) return 2;
    if (!(memcmp("abd", "abc", 3) > 0)) return 3;
    /* only the first n bytes participate */
    if (memcmp("abX", "abY", 2) != 0) return 4;
    /* n == 0 compares equal */
    if (memcmp("a", "b", 0) != 0) return 5;
    /* bytes compare as unsigned char: 0x80 > 0x01 */
    char a[1] = {(char)0x80}, b[1] = {0x01};
    if (!(memcmp(a, b, 1) > 0)) return 6;
    return 0;
}
