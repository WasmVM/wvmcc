/* LIBC-stdio-sprintf-01 — C17 7.21.6.6: sprintf writes formatted output
 * to a buffer, appends a null character, and returns the number of
 * characters written (not counting the null). Verify=exit. */
#include <stdio.h>
int main(void) {
    char buf[16];
    int n = sprintf(buf, "%d-%s", 42, "ab");  /* "42-ab" */
    if (n != 5) return 1;
    if (buf[0] != '4') return 2;
    if (buf[1] != '2') return 3;
    if (buf[2] != '-') return 4;
    if (buf[3] != 'a') return 5;
    if (buf[4] != 'b') return 6;
    if (buf[5] != '\0') return 7;
    return 0;
}
