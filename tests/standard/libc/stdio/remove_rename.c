/* LIBC-stdio-remove-01 — C17 7.21.4: remove deletes a file (the name no
 * longer opens it), rename changes a file's name, tmpfile creates a
 * temporary binary stream, tmpnam yields a valid file name. Verify=exit. */
#include <stdio.h>
int main(void) {
    FILE *f = fopen("remove_a.tmp", "w");
    if (!f) return 1;
    if (fputc('x', f) != 'x') return 2;
    if (fclose(f) != 0) return 3;

    /* rename: old name gone, new name has the contents. */
    if (rename("remove_a.tmp", "remove_b.tmp") != 0) return 4;
    if (fopen("remove_a.tmp", "r") != 0) return 5;
    f = fopen("remove_b.tmp", "r");
    if (!f) return 6;
    if (fgetc(f) != 'x') return 7;
    fclose(f);

    /* remove: the name no longer opens the file. */
    if (remove("remove_b.tmp") != 0) return 8;
    if (fopen("remove_b.tmp", "r") != 0) return 9;

    /* tmpfile: a usable wb+ stream, removed at close/termination. */
    FILE *t = tmpfile();
    if (!t) return 10;
    if (fputc('t', t) != 't') return 11;
    if (fclose(t) != 0) return 12;

    /* tmpnam: returns a valid file name (non-null). */
    if (tmpnam(0) == 0) return 13;
    return 0;
}
