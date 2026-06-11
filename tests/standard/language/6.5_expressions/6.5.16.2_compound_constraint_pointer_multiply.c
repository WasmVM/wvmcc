/* LANG-6.5.16.2-02 — Constraint: for compound assignment operators other than
 * `+=` and `-=`, both operands must have arithmetic type (ISO C17
 * 6.5.16.2p1,p2). `+=`/`-=` additionally permit a pointer LHS with an integer
 * RHS, but `*=` does not. Applying `*=` to a pointer left operand must be
 * rejected by a conforming compiler. */

int main(void)
{
    int x = 0;
    int *p = &x;

    p *= 2;   /* ill-formed: `*=` requires an arithmetic left operand */

    return 0;
}
