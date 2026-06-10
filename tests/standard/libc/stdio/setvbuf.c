/* LIBC-stdio-setvbuf-01 — C17 7.21.5.5/7.21.5.6: setvbuf sets the
 * buffering mode of a stream (only before any other operation on it) and
 * returns zero on success; setbuf is equivalent to setvbuf with _IOFBF /
 * BUFSIZ (or _IONBF for a null buffer). Verify=stdout. */
#include <stdio.h>
static char vb[256];
int main(void) {
    if (setvbuf(stdout, vb, _IOFBF, sizeof vb) != 0) return 1;
    printf("buffered\n");           /* held in vb until flush/exit */

    FILE *f = fopen("setvbuf_test.tmp", "w");
    if (!f) return 2;
    setbuf(f, 0);                   /* _IONBF: unbuffered, returns void */
    if (fputc('u', f) != 'u') return 3;
    if (fclose(f) != 0) return 4;

    f = fopen("setvbuf_test.tmp", "r");
    if (!f) return 5;
    if (fgetc(f) != 'u') return 6;
    fclose(f);
    remove("setvbuf_test.tmp");

    printf("done\n");               /* flushed at normal termination */
    return 0;
}
