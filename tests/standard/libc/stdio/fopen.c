/* LIBC-stdio-fopen-01 — C17 7.21.5.3: fopen opens the named file in the
 * given mode and returns a stream, or a null pointer on failure.
 * Verify=exit. */
#include <stdio.h>
int main(void) {
    FILE *f = fopen("fopen_test.tmp", "w");
    if (!f) return 1;
    if (fputs("ab", f) < 0) return 2;
    if (fclose(f) != 0) return 3;

    f = fopen("fopen_test.tmp", "r");
    if (!f) return 4;
    if (fgetc(f) != 'a') return 5;
    if (fgetc(f) != 'b') return 6;
    if (fgetc(f) != EOF) return 7;
    fclose(f);
    remove("fopen_test.tmp");

    /* Opening a nonexistent file for reading must fail with NULL. */
    if (fopen("fopen_no_such_file.tmp", "r") != 0) return 8;
    return 0;
}
