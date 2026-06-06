/* tests/standard/libc/ctype/iscntrl.c — LIBC-ctype-iscntrl-01. Verify=exit. */
#include <ctype.h>
int main(void) {
    if (!iscntrl('\0') || !iscntrl('\n') || !iscntrl('\t') || !iscntrl(127)) return 1;
    if (iscntrl(' ') || iscntrl('A') || iscntrl('~')) return 2;
    return 0;
}
