/* LANG-6.2.3-02 — 6.2.3p1: a label name occupies its own name space, distinct
 * from ordinary identifiers, so a label may share its spelling with a variable. */
int main(void) {
    int again = 5;
    goto again;
again:
    if (again != 5) return 1;
    return 0;
}
