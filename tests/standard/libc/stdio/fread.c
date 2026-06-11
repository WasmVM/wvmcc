/* LIBC-stdio-fread-01 — C17 7.21.8.1: fread reads up to nmemb elements of
 * the given size and returns the number of complete elements read, which
 * is less than nmemb only on error or end-of-file. Verify=exit. */
#include <stdio.h>
int main(void) {
    FILE *f = fopen("fread_test.tmp", "wb");
    if (!f) return 1;
    if (fputs("abcd", f) < 0) return 2;
    if (fclose(f) != 0) return 3;

    f = fopen("fread_test.tmp", "rb");
    if (!f) return 4;
    char b[8];
    if (fread(b, 1, 4, f) != 4) return 5;
    if (b[0] != 'a' || b[1] != 'b' || b[2] != 'c' || b[3] != 'd') return 6;
    if (fread(b, 1, 4, f) != 0) return 7;   /* at EOF: short count */
    fclose(f);
    remove("fread_test.tmp");
    return 0;
}
