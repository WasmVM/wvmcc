/* tests/standard/libc/string/memset.c — LIBC-string-memset-01. ISO C17 §7.24.6.1. Verify=exit.
 * memset fills n bytes of dst with (unsigned char)c and returns dst. */
#include <string.h>
int main(void) {
    char buf[5] = {'X', 'X', 'X', 'X', 'X'};
    if (memset(buf, 'a', 3) != buf) return 1;
    if (buf[0] != 'a' || buf[1] != 'a' || buf[2] != 'a') return 2;
    if (buf[3] != 'X' || buf[4] != 'X') return 3; /* beyond n untouched */
    /* value converted to unsigned char: 0x141 -> 0x41 ('A') */
    memset(buf, 0x141, 1);
    if (buf[0] != 0x41) return 4;
    /* n == 0 writes nothing */
    memset(buf, 'z', 0);
    if (buf[0] != 0x41) return 5;
    return 0;
}
