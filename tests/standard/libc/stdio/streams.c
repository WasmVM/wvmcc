/* LIBC-stdio-streams-01 — C17 7.21.1p3: stdin, stdout, stderr are
 * expressions of type FILE * designating the predefined streams.
 * Verify=stdout. stderr output is not captured; the test only checks
 * that all three are usable FILE * expressions and that stdout works. */
#include <stdio.h>
int main(void) {
    FILE *in = stdin;   /* must be a valid FILE * expression */
    FILE *err = stderr; /* must be a valid FILE * expression */
    (void)in;
    if (fputs("via stdout\n", stdout) < 0) return 1;
    if (fputs("via stderr\n", err) < 0) return 2;
    return 0;
}
