/* LANG-5.1.2.3-03 — 5.1.2.3p6 (with 7.21.3 buffering dynamics): output to a
 * line-buffered or unbuffered stream appears as soon as a line is completed.
 * Each completed line written to stdout must appear, in order, in the
 * program's output. */
#include <stdio.h>

int main(void)
{
    printf("line 1\n");         /* completed line: delivered promptly */
    puts("line 2");             /* puts appends the newline */
    printf("line %d\n", 3);
    return 0;
}
