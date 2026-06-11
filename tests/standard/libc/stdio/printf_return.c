/* LIBC-stdio-printf-01 — C17 7.21.6.3: printf writes to stdout and
 * returns the number of characters transmitted. Verify=stdout. */
#include <stdio.h>
int main(void) {
    int n = printf("abc\n");           /* 4 characters */
    if (n != 4) return 1;
    n = printf("n=%d\n", n);           /* "n=4\n" -> 4 characters */
    if (n != 4) return 2;
    return 0;
}
