/* LIBC-stdio-snprintf-01 — C17 7.21.6.5: snprintf writes at most n-1
 * characters plus a null terminator and returns the number of characters
 * that WOULD have been written had n been large enough. With n == 0,
 * nothing is written (buffer may be null). Verify=exit. */
#include <stdio.h>
int main(void) {
    char buf[4];
    int n = snprintf(buf, sizeof buf, "%d", 12345);
    if (n != 5) return 1;                 /* would-be length */
    if (buf[0] != '1') return 2;          /* truncated to 3 chars + NUL */
    if (buf[1] != '2') return 3;
    if (buf[2] != '3') return 4;
    if (buf[3] != '\0') return 5;
    n = snprintf(0, 0, "%s!", "hey");     /* n==0: nothing written */
    if (n != 4) return 6;
    return 0;
}
