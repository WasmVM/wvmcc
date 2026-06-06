/* tests/standard/libc/ctype/isgraph.c — LIBC-ctype-isgraph-01. Verify=exit. */
#include <ctype.h>
int main(void) {
    if (!isgraph('A') || !isgraph('~') || !isgraph('0') || !isgraph('!')) return 1;
    if (isgraph(' ') || isgraph('\n') || isgraph(127)) return 2;
    return 0;
}
