/* LANG-6.2.1-03 — 6.2.1p4: a block-scope identifier is not visible after
 * its block ends; referencing it there is a constraint violation (6.5.1p2). */
int f(void)
{
    {
        int inner = 0;
        (void)inner;
    }
    return inner;   /* ill-formed: 'inner' is out of scope here */
}
