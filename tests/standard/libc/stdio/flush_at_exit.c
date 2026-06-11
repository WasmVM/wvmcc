/* LIBC-stdio-flush-exit-01 — C17 5.1.2.3p6 / 7.22.4.4: at normal program
 * termination all open streams with unwritten buffered data are flushed.
 * No newline and no explicit fflush: the bytes must still appear.
 * Verify=stdout. */
#include <stdio.h>
int main(void) {
    printf("flushed at exit");
    return 0;
}
