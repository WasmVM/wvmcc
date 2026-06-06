/* tests/standard/libc/ctype/isalpha.c — LIBC-ctype-isalpha-01. Verify=exit. */
#include <ctype.h>
int main(void) {
    for (int c = 'A'; c <= 'Z'; c++) if (!isalpha(c)) return 1;
    for (int c = 'a'; c <= 'z'; c++) if (!isalpha(c)) return 2;
    if (isalpha('0') || isalpha(' ') || isalpha('_')) return 3;
    return 0;
}
