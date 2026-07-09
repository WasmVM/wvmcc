/* A goto into a switch body is legal C (6.8.6.1p1 — Duff's device) but is
 * not lowered by wvmcc: the case-segment lowering has no inward goto routing.
 * Documented limitation (catalog LANG-6.8.6.1-05 note); rejected with
 * "goto into a switch body is not supported" rather than misrouting. */
int main(void)
{
    int x = 1;
    goto in;
    switch (x) {
    case 1: { in: return 0; }
    }
    return 1;
}
