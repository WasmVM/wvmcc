/* LIBC-stdio-fwrite-01 — C17 7.21.8.2: fwrite writes nmemb elements of
 * the given size from the array and returns the number of complete
 * elements written. Verify=exit. */
#include <stdio.h>
int main(void) {
    const char d[4] = { 'a', 'b', 'c', 'd' };
    FILE *f = fopen("fwrite_test.tmp", "wb");
    if (!f) return 1;
    if (fwrite(d, 2, 2, f) != 2) return 2;  /* 2 elements of size 2 */
    if (fclose(f) != 0) return 3;

    f = fopen("fwrite_test.tmp", "rb");
    if (!f) return 4;
    if (fgetc(f) != 'a') return 5;
    if (fgetc(f) != 'b') return 6;
    if (fgetc(f) != 'c') return 7;
    if (fgetc(f) != 'd') return 8;
    if (fgetc(f) != EOF) return 9;
    fclose(f);
    remove("fwrite_test.tmp");
    return 0;
}
