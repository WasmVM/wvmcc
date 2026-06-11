/* LANG-6.2.2-02 — 6.2.2p4: if a declaration of an identifier uses `extern`
 * and a prior declaration of that identifier is visible with internal or
 * external linkage, the linkage at the later declaration is the same as the
 * prior one. So an `extern` redeclaration of a prior `static` object keeps
 * INTERNAL linkage (it does not become external).
 *
 * Single-TU `exit` test: the `extern` redeclaration must refer to the same
 * private object, observable by address and by value.
 */

static int s = 7;                 /* prior declaration: internal linkage */

int main(void) {
    extern int s;                 /* 6.2.2p4: keeps the prior internal linkage */

    if (s != 7) return 1;         /* refers to the same object */

    s = 42;                       /* mutate via the extern-redeclared name */
    {
        extern int s;             /* same identifier, same object */
        if (s != 42) return 2;
        if (&s == 0) return 3;
    }

    /* Confirm both names denote one object: take the address two ways. */
    int *p = &s;
    *p = 100;
    if (s != 100) return 4;

    return 0;
}
