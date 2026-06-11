/* LANG-6.5.16.2-02 — Compound-assignment constraints (ISO C17 6.5.16.2p1,p2):
 * for `+=` and `-=` the left operand may be a pointer (with integer right
 * operand) or arithmetic; for all other compound operators (here `*=`) both
 * operands must be arithmetic. Applying `*=` to a pointer left operand violates
 * the constraint; a conforming compiler must reject this. */

int f(int *p)
{
    p *= 2;   /* ill-formed: `*=` requires an arithmetic left operand */
    return *p;
}
