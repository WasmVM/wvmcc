/* tests/standard/libc/ctype/isupper.c — LIBC-ctype-isupper-01. Verify=exit. */
#include <ctype.h>
int main(void) {
    for (int c = 'A'; c <= 'Z'; c++) if (!isupper(c)) return 1;
    if (isupper('a') || isupper('0') || isupper(' ')) return 2;
    return 0;
}
