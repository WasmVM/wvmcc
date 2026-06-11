/* LIBC-stdio-fgets-01 — C17 7.21.7.2: fgets reads at most n-1 characters,
 * stopping after a newline (which is retained), always null-terminates,
 * and returns a null pointer at end-of-file with nothing read.
 * Verify=exit. */
#include <stdio.h>
int main(void) {
    FILE *f = fopen("fgets_test.tmp", "w");
    if (!f) return 1;
    if (fputs("abc\n", f) < 0) return 2;
    if (fclose(f) != 0) return 3;

    f = fopen("fgets_test.tmp", "r");
    if (!f) return 4;
    char buf[8];

    /* Bounded read: n==3 reads at most 2 chars + NUL. */
    if (fgets(buf, 3, f) != buf) return 5;
    if (buf[0] != 'a' || buf[1] != 'b' || buf[2] != '\0') return 6;

    /* Rest of the line: newline retained, then NUL. */
    if (fgets(buf, sizeof buf, f) != buf) return 7;
    if (buf[0] != 'c' || buf[1] != '\n' || buf[2] != '\0') return 8;

    /* At end-of-file with no characters read: NULL. */
    if (fgets(buf, sizeof buf, f) != 0) return 9;
    fclose(f);
    remove("fgets_test.tmp");
    return 0;
}
