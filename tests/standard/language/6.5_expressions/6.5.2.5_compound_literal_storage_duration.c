/* LANG-6.5.2.5-02 — A compound literal that occurs outside the body of a
 * function has static storage duration; one that occurs inside a function body
 * has automatic storage duration associated with the enclosing block
 * (ISO C17 6.5.2.5p5). */

/* File-scope compound literal: static storage duration, so its address is
 * usable for the lifetime of the program and may initialize another static. */
static int *const g = (int[]){ 1, 2, 3 };

static int block_scope_value(void)
{
    /* Block-scope compound literal: a NEW object each time the block is entered
     * (automatic storage duration), re-initialized to { 100, 200 } on entry.
     * We read it while it is alive and return only the resulting value. */
    int *p = (int[]){ 100, 200 };
    p[0] += 1;
    return p[0];
}

int main(void)
{
    /* The file-scope literal persists and keeps its initialized contents. */
    if (g[0] != 1 || g[1] != 2 || g[2] != 3) return 1;

    /* Each entry to the block re-initializes the automatic compound literal,
     * so the observed value is independent of any previous call. */
    if (block_scope_value() != 101) return 2;
    if (block_scope_value() != 101) return 3;   /* re-init to 100, then +1 */

    return 0;
}
