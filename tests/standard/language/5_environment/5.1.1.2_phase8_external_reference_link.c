/* LANG-5.1.1.2-08 — 5.1.1.2p1(8): translation phase 8 — external references
 * are resolved and the program is linked into an executable image.
 * A reference to an external-linkage function declared before its definition
 * must resolve to that definition in the program image. */
int add(int a, int b);          /* external reference, resolved at phase 8 */

static int call_through(int x)
{
    return add(x, 2);
}

int add(int a, int b)
{
    return a + b;
}

int main(void)
{
    if (add(1, 2) != 3) return 1;
    if (call_through(40) != 42) return 2;
    return 0;
}
