/* tests/standard/libc/ctype/isblank.c — LIBC-ctype-isblank-01. ISO C17 §7.4.1.3. Verify=exit.
 * isblank: true for the standard blank characters space (' ') and horizontal tab ('\t'). */
#include <ctype.h>
int main(void) {
    if (!isblank(' ')) return 1;
    if (!isblank('\t')) return 2;
    if (isblank('\n') || isblank('\v') || isblank('\f') || isblank('\r')) return 3;
    if (isblank('a') || isblank('0') || isblank('_')) return 4;
    return 0;
}
