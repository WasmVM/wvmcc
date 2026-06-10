/* LIBC-stdio-error-01 — C17 7.21.10: feof / ferror report the stream's
 * end-of-file and error indicators; clearerr clears both; perror writes
 * an errno message to stderr (linkage exercised only — stderr is not
 * captured by exit verification). Verify=exit. */
#include <stdio.h>
int main(void) {
    FILE *f = fopen("error_test.tmp", "w");
    if (!f) return 1;
    if (fputc('x', f) != 'x') return 2;
    if (fclose(f) != 0) return 3;

    f = fopen("error_test.tmp", "r");
    if (!f) return 4;
    if (feof(f)) return 5;        /* fresh stream: indicators clear */
    if (ferror(f)) return 6;
    if (fgetc(f) != 'x') return 7;
    if (fgetc(f) != EOF) return 8;
    if (!feof(f)) return 9;       /* EOF indicator now set */
    clearerr(f);
    if (feof(f)) return 10;       /* cleared */
    if (ferror(f)) return 11;
    fclose(f);
    remove("error_test.tmp");

    perror("stream_error");       /* must be callable; writes to stderr */
    return 0;
}
