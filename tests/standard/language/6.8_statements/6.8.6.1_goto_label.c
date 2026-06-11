/* LANG-6.8.6.1-01 — A `goto` statement causes an unconditional jump to the
 * statement prefixed by the named label in the enclosing function
 * (ISO C17 6.8.6.1p2). Tests forward and backward jumps. */

int main(void)
{
    int trace = 0;

    /* Forward jump: the skipped statement must not execute. */
    goto fwd;
    trace |= 1;             /* must be skipped */
fwd:
    trace |= 2;
    if (trace != 2) return 1;

    /* Backward jump: build a loop out of goto. */
    int i = 0;
back:
    i++;
    if (i < 4) goto back;
    if (i != 4) return 2;

    return 0;
}
