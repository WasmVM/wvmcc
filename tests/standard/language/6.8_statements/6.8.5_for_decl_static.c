/* LANG-6.8.5-02 — Constraint: the declaration part of a `for` statement
 * shall only declare identifiers for objects having storage class `auto` or
 * `register` (ISO C17 6.8.5p3). Declaring a `static` object in clause-1 is
 * a constraint violation a conforming compiler must reject. */

int f(void)
{
    int sum = 0;
    for (static int i = 0; i < 3; i++) {  /* ill-formed: static in for clause-1 */
        sum += i;
    }
    return sum;
}
