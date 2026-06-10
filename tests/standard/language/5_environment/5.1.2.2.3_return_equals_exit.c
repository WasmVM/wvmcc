/* LANG-5.1.2.2.3-01 — 5.1.2.2.3p1: a `return` from the initial call to main
 * is equivalent to calling exit() with the returned value — the value becomes
 * the program's termination status. Returning a computed 0 must terminate
 * with exit status 0. */
static int compute_status(void)
{
    int n = 6 * 7;
    return n - 42;              /* 0 — termination status must be 0 */
}

int main(void)
{
    return compute_status();
}
