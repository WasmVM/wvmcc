/* LANG-6.2.4-03 — 6.2.4p6: a block-scope object declared with `static` has a
 * single instance that retains its value across calls to the function. */

static int next_id(void) {
    static int n = 0;   /* one instance, initialized once */
    n = n + 1;
    return n;
}

int main(void) {
    if (next_id() != 1) return 1;
    if (next_id() != 2) return 2;
    if (next_id() != 3) return 3;
    return 0;
}
