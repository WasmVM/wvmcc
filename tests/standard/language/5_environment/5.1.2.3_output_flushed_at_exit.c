/* LANG-5.1.2.3-02 — 5.1.2.3p6: at program termination, all data written into
 * files (here: stdout) shall be identical to the result the abstract
 * semantics would have produced — i.e. buffered output is flushed at exit. */
#include <stdio.h>

int main(void)
{
    printf("first\n");
    printf("second");           /* no trailing newline: must still be flushed */
    printf(" and third\n");
    return 0;                   /* normal termination flushes streams */
}
