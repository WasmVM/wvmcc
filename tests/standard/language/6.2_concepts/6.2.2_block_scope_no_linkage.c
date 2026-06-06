/* LANG-6.2.2-05 — 6.2.2p6: an identifier for an object declared at block scope
 * with no storage-class specifier (and no `extern`) has NO linkage. Each such
 * declaration denotes a unique, distinct object; the same name in a different
 * block, or on a re-entry of the same block, is a separate object.
 *
 * Single-TU `exit` test: distinct block-scope objects with the same name occupy
 * distinct storage, and a no-linkage object is fresh on each function entry.
 */

static int fresh_each_call(void) {
    int local = 0;                /* no linkage: distinct object per entry */
    local += 1;
    return local;                 /* always 1 — never carries across calls */
}

int main(void) {
    int x = 1;                    /* no-linkage object A */
    {
        int x = 2;                /* no-linkage object B: distinct from A */
        if (x != 2) return 1;
        if (&x == 0) return 2;
        int *inner = &x;
        {
            int x = 3;            /* no-linkage object C: distinct again */
            if (x != 3) return 3;
            if (&x == inner) return 4;   /* C and B are different objects */
        }
        if (x != 2) return 5;     /* B is unaffected by C */
        if (inner != &x) return 6;
    }
    if (x != 1) return 7;         /* outer A unaffected by inner B/C */

    /* A no-linkage automatic object is a fresh, distinct object each entry. */
    if (fresh_each_call() != 1) return 8;
    if (fresh_each_call() != 1) return 9;

    return 0;
}
