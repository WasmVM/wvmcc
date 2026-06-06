/* tests/standard/libc/ctype/isspace.c — LIBC-ctype-isspace-01. Verify=exit. */
#include <ctype.h>
int main(void) {
    if (!isspace(' ') || !isspace('\t') || !isspace('\n')
        || !isspace('\v') || !isspace('\f') || !isspace('\r')) return 1;
    if (isspace('a') || isspace('0') || isspace('_')) return 2;
    return 0;
}
