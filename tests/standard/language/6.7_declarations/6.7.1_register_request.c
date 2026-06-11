/* LANG-6.7.1-05 — effectiveness of a `register` request is
 * implementation-defined (C17 6.7.1p6).  Whatever the implementation decides
 * (wvmcc treats `register` as `auto`, per docs/spec.md), a register object
 * must still behave as an ordinary object of its type: it can be read,
 * written, used in arithmetic, and sizeof applies. */
int main(void) {
    register int r = 10;
    register unsigned long ul = 100;

    r = r + 5;
    if (r != 15) return 1;

    ul *= 2;
    if (ul != 200UL) return 2;

    if (sizeof r != sizeof(int)) return 3;

    {
        register int inner = r;
        inner -= 15;
        if (inner != 0) return 4;
    }
    return 0;
}
