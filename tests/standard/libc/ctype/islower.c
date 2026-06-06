/* tests/standard/libc/ctype/islower.c — LIBC-ctype-islower-01. Verify=exit. */
#include <ctype.h>
int main(void) {
    for (int c = 'a'; c <= 'z'; c++) if (!islower(c)) return 1;
    if (islower('A') || islower('0') || islower(' ')) return 2;
    return 0;
}
