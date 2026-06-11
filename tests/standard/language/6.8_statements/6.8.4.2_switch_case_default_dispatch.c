/* LANG-6.8.4.2-01 — `switch`: control jumps to the case whose constant equals
 * the converted controlling expression; if no case matches and there is a
 * `default` label, control jumps there; otherwise control passes beyond the
 * whole switch body (ISO C17 6.8.4.2p4, p5, p7). */

static int classify(int v)
{
    switch (v) {
    case 1:
        return 10;
    case 2:
        return 20;
    default:
        return 99;
    }
}

static int no_default(int v)
{
    int r = -1;
    switch (v) {
    case 7:
        r = 70;
        break;
    }
    return r; /* no match, no default: body is skipped entirely */
}

int main(void)
{
    if (classify(1) != 10) return 1;
    if (classify(2) != 20) return 2;
    if (classify(3) != 99) return 3;   /* default taken */
    if (classify(-5) != 99) return 4;

    if (no_default(7) != 70) return 5;
    if (no_default(8) != -1) return 6; /* past the body */

    /* The controlling expression undergoes integer promotion and the case
     * constants are converted to that type before comparison (6.8.4.2p5). */
    char c = 2;
    switch (c) {
    case 2L: /* long constant converted to the promoted type, matches */
        return 0;
    default:
        return 7;
    }
}
