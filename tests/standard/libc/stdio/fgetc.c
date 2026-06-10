/* LIBC-stdio-fgetc-01 — C17 7.21.7.1/7.21.7.5/7.21.7.10: fgetc and getc
 * read the next character as unsigned char converted to int (EOF at end
 * of file); ungetc pushes a character back, which the next read returns.
 * Verify=exit. (getchar reads stdin and needs harness-provided input, so
 * it is exercised only via its declaration here.) */
#include <stdio.h>
int main(void) {
    int (*gc)(void) = getchar;     /* must be declared with this type */
    (void)gc;

    FILE *f = fopen("fgetc_test.tmp", "w");
    if (!f) return 1;
    if (fputs("xy", f) < 0) return 2;
    if (fclose(f) != 0) return 3;

    f = fopen("fgetc_test.tmp", "r");
    if (!f) return 4;
    int c = fgetc(f);
    if (c != 'x') return 5;
    if (ungetc(c, f) != 'x') return 6;  /* pushback returns the char */
    if (getc(f) != 'x') return 7;       /* read returns pushed-back char */
    if (getc(f) != 'y') return 8;
    if (fgetc(f) != EOF) return 9;
    fclose(f);
    remove("fgetc_test.tmp");
    return 0;
}
