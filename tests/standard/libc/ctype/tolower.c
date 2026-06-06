/* tests/standard/libc/ctype/tolower.c — LIBC-ctype-tolower-01. Verify=exit. */
#include <ctype.h>
int main(void) {
    if (tolower('A') != 'a' || tolower('Z') != 'z') return 1;
    if (tolower('a') != 'a' || tolower('0') != '0' || tolower('!') != '!') return 2;
    return 0;
}
