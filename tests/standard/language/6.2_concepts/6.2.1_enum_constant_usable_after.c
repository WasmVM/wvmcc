/* LANG-6.2.1-06 — 6.2.1p7: an enumeration constant's scope begins just after
 * its defining enumerator, so a later enumerator may use the earlier one. */
enum E {
    A = 3,
    B = A + 1,     /* A is in scope from just after its enumerator */
    C = B + A
};

int main(void)
{
    if (A != 3) return 1;
    if (B != 4) return 2;
    if (C != 7) return 3;
    return 0;
}
