/* LIBC-stdio-freopen-01 — C17 7.21.5.4: freopen first closes the file
 * associated with the stream, then opens the new file and associates the
 * stream with it; returns the stream value or a null pointer on failure.
 * Verify=exit. */
#include <stdio.h>
int main(void) {
    FILE *f = fopen("freopen_a.tmp", "w");
    if (!f) return 1;
    if (fputc('A', f) != 'A') return 2;

    FILE *g = freopen("freopen_b.tmp", "w", f);
    if (!g) return 3;
    if (fputc('B', g) != 'B') return 4;
    if (fclose(g) != 0) return 5;

    /* The old file was closed (and flushed): it must contain 'A'. */
    f = fopen("freopen_a.tmp", "r");
    if (!f) return 6;
    if (fgetc(f) != 'A') return 7;
    fclose(f);

    /* The new file got the post-freopen write. */
    f = fopen("freopen_b.tmp", "r");
    if (!f) return 8;
    if (fgetc(f) != 'B') return 9;
    fclose(f);

    remove("freopen_a.tmp");
    remove("freopen_b.tmp");
    return 0;
}
