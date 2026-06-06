/* tests/standard/libc/ctype/isxdigit.c — LIBC-ctype-isxdigit-01. Verify=exit. */
#include <ctype.h>
int main(void) {
    if (!isxdigit('0') || !isxdigit('9') || !isxdigit('a') || !isxdigit('f')
        || !isxdigit('A') || !isxdigit('F')) return 1;
    if (isxdigit('g') || isxdigit('G') || isxdigit(' ')) return 2;
    return 0;
}
