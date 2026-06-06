/* tests/standard/libc/ctype/toupper.c — LIBC-ctype-toupper-01. Verify=exit. */
#include <ctype.h>
int main(void) {
    if (toupper('a') != 'A' || toupper('z') != 'Z') return 1;
    if (toupper('A') != 'A' || toupper('0') != '0' || toupper('!') != '!') return 2;
    return 0;
}
