/* LIBC-stdio-fseek-01 — C17 7.21.9: fseek / ftell / rewind / fgetpos /
 * fsetpos position a stream. Binary mode is used so byte offsets are
 * well-defined for SEEK_SET/SEEK_CUR/SEEK_END. Verify=exit. */
#include <stdio.h>
int main(void) {
    FILE *f = fopen("fseek_test.tmp", "wb");
    if (!f) return 1;
    if (fputs("abcd", f) < 0) return 2;
    if (fclose(f) != 0) return 3;

    f = fopen("fseek_test.tmp", "rb");
    if (!f) return 4;

    if (fseek(f, 2, SEEK_SET) != 0) return 5;
    if (ftell(f) != 2L) return 6;
    if (fgetc(f) != 'c') return 7;

    fpos_t pos;                          /* now at offset 3 */
    if (fgetpos(f, &pos) != 0) return 8;
    if (fgetc(f) != 'd') return 9;
    if (fsetpos(f, &pos) != 0) return 10;
    if (fgetc(f) != 'd') return 11;      /* restored position */

    if (fseek(f, -1, SEEK_END) != 0) return 12;
    if (fgetc(f) != 'd') return 13;

    rewind(f);
    if (ftell(f) != 0L) return 14;
    if (fgetc(f) != 'a') return 15;

    fclose(f);
    remove("fseek_test.tmp");
    return 0;
}
