/* LANG-6.8.4.2-04 — Case and default labels do not alter the flow of control:
 * without a `break`, execution falls through from one case's statements into
 * the next (ISO C17 6.8.4.2p4 and footnote; cf. 6.8.4.2p7 EXAMPLE). */

static int run(int v)
{
    int acc = 0;
    switch (v) {
    case 0:
        acc += 1;
        /* fall through */
    case 1:
        acc += 10;
        /* fall through */
    case 2:
        acc += 100;
        break;
    case 3:
        acc += 1000;
        /* fall through into default */
    default:
        acc += 10000;
    }
    return acc;
}

int main(void)
{
    if (run(0) != 111) return 1;      /* case 0 -> 1 -> 2 */
    if (run(1) != 110) return 2;      /* case 1 -> 2 */
    if (run(2) != 100) return 3;      /* case 2 only (break stops it) */
    if (run(3) != 11000) return 4;    /* case 3 falls into default */
    if (run(42) != 10000) return 5;   /* default only */
    return 0;
}
