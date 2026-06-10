/* LIBC-stdio-fprintf-01 — C17 7.21.6.1: fprintf writes formatted output
 * to the given stream and returns the number of characters transmitted.
 * Verify=stdout. */
#include <stdio.h>
int main(void) {
    int n = fprintf(stdout, "%s=%d\n", "x", 7);  /* "x=7\n" -> 4 */
    if (n != 4) return 1;
    return 0;
}
