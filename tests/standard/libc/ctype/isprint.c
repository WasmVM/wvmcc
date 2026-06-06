/* tests/standard/libc/ctype/isprint.c — LIBC-ctype-isprint-01. Verify=exit. */
#include <ctype.h>
int main(void) {
    if (!isprint(' ') || !isprint('A') || !isprint('~') || !isprint('0')) return 1;
    if (isprint('\n') || isprint('\t') || isprint(127)) return 2;
    return 0;
}
