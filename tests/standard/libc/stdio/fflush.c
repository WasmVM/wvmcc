/* LIBC-stdio-fflush-01 — C17 7.21.5.2: fflush delivers unwritten buffered
 * data for an output stream to the host environment and returns zero on
 * success. Verify=stdout. */
#include <stdio.h>
int main(void) {
    printf("before");
    if (fflush(stdout) != 0) return 1;
    printf(" after\n");
    if (fflush(0) != 0) return 2;   /* NULL: flush all output streams */
    return 0;
}
