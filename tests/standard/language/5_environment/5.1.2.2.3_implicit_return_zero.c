/* LANG-5.1.2.2.3-02 — 5.1.2.2.3p1: reaching the closing `}` of main
 * terminates the program with status 0 (as if `return 0;`). */
int main(void)
{
    int x = 1;
    x = x + 1;                  /* some work; then fall off the end */
}
