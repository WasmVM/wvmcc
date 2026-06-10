/* tests/standard/libc/string/strtok.c — LIBC-string-strtok-01. ISO C17 §7.24.5.8. Verify=exit.
 * strtok tokenizes in place using a separator set; subsequent calls pass NULL
 * to continue. Leading separators are skipped; returns NULL when exhausted. */
#include <string.h>
int main(void) {
    char buf[10] = ",ab,,c,";
    char *t = strtok(buf, ",");
    if (t == NULL || strcmp(t, "ab") != 0) return 1; /* leading seps skipped */
    t = strtok(NULL, ",");
    if (t == NULL || strcmp(t, "c") != 0) return 2;  /* empty fields skipped */
    if (strtok(NULL, ",") != NULL) return 3;         /* exhausted */
    /* a string of only separators yields no tokens */
    char b2[4] = ",,,";
    if (strtok(b2, ",") != NULL) return 4;
    return 0;
}
