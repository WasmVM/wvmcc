/* tests/standard/libc/ctype/isalnum.c — LIBC-ctype-isalnum-01. Verify=exit. */
#include <ctype.h>
int main(void) {
    if (!isalnum('a') || !isalnum('Z') || !isalnum('5')) return 1;
    if (isalnum(' ') || isalnum('!') || isalnum('\n')) return 2;
    return 0;
}
