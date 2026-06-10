/* LANG-6.7-02 — a no-linkage identifier is declared at most once per
 * scope/name-space (6.7p3): redeclaring a block-scope object is rejected. */
int main(void)
{
    int x;
    int x; /* constraint violation: second declaration of no-linkage `x` */
    return 0;
}
