/* LANG-6.2.1-02 — 6.2.1p4: an inner block-scope declaration hides an
 * outer same-name declaration; the outer is visible again after the block. */
int main(void)
{
    int x = 1;          /* outer */
    {
        int x = 2;      /* hides outer within this block */
        if (x != 2) return 1;
    }
    if (x != 1) return 2;   /* outer restored after the block */
    return 0;
}
