/* tests/standard/libc/string/memmove.c — LIBC-string-memmove-01. ISO C17 §7.24.2.2. Verify=exit.
 * memmove copies correctly even when the regions overlap; returns dst. */
#include <string.h>
int main(void) {
    char buf[8] = {'a', 'b', 'c', 'd', 'e', '\0', 0, 0};
    /* overlap: shift right by 2 -> "ababcde" prefix region */
    if (memmove(buf + 2, buf, 5) != buf + 2) return 1;
    if (buf[2] != 'a' || buf[3] != 'b' || buf[4] != 'c' || buf[5] != 'd' || buf[6] != 'e') return 2;
    if (buf[0] != 'a' || buf[1] != 'b') return 3;
    /* overlap: shift left by 1 */
    char b2[5] = {'1', '2', '3', '4', '\0'};
    memmove(b2, b2 + 1, 4);
    if (b2[0] != '2' || b2[1] != '3' || b2[2] != '4' || b2[3] != '\0') return 4;
    return 0;
}
