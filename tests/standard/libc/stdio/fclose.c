/* LIBC-stdio-fclose-01 — C17 7.21.5.1: fclose flushes the stream
 * (delivering any unwritten buffered data to the host environment) and
 * closes it, returning zero on success. Verify=exit. */
#include <stdio.h>
int main(void) {
    FILE *f = fopen("fclose_test.tmp", "w");
    if (!f) return 1;
    if (fputc('x', f) != 'x') return 2;
    if (fclose(f) != 0) return 3;     /* must flush + succeed */

    /* The buffered byte must now be visible in the file. */
    f = fopen("fclose_test.tmp", "r");
    if (!f) return 4;
    if (fgetc(f) != 'x') return 5;
    if (fclose(f) != 0) return 6;
    remove("fclose_test.tmp");
    return 0;
}
