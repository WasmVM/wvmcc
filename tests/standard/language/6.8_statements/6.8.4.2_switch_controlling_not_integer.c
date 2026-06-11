/* LANG-6.8.4.2-03 — Constraint: the controlling expression of a `switch`
 * statement shall have integer type (ISO C17 6.8.4.2p1). A floating type is
 * not an integer type, so a conforming compiler must reject this. */

int main(void)
{
    double d = 1.0;
    switch (d) { /* error: controlling expression has floating type */
    case 1:
        return 0;
    default:
        return 1;
    }
}
