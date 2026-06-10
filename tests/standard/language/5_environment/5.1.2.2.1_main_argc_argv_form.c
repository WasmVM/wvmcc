/* LANG-5.1.2.2.1-02 — 5.1.2.2.1p1: the form
 * `int main(int argc, char *argv[])` shall be accepted, and the program runs
 * to successful termination. */
int main(int argc, char *argv[])
{
    (void)argv;
    if (argc < 0) return 1;     /* argc shall be nonnegative (p2) */
    return 0;
}
