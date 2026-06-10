/* LANG-6.7.2.2-01 — ISO C17 §6.7.2.2p3
 * Enumerators have type int. Without '=', each enumerator's value is one
 * greater than the previous (first defaults to 0); '=' overrides the value;
 * distinct enumerators may share the same value.
 */

enum color { RED, GREEN, BLUE = 10, CYAN, ALSO_TEN = 10, NEG = -1, ZERO_AGAIN };

/* Enumerators are constants of type int (§6.7.2.2p3). */
_Static_assert(sizeof RED == sizeof(int), "enumerator has type int");

int main(void)
{
    if (RED != 0) return 1;          /* first enumerator defaults to 0 */
    if (GREEN != 1) return 2;        /* sequential: previous + 1 */
    if (BLUE != 10) return 3;        /* '=' overrides */
    if (CYAN != 11) return 4;        /* sequence resumes after override */
    if (ALSO_TEN != BLUE) return 5;  /* duplicate values are allowed */
    if (NEG != -1) return 6;         /* negative ICE values are allowed */
    if (ZERO_AGAIN != 0) return 7;   /* -1 + 1 == 0 */
    if (RED - 1 != -1) return 8;     /* int (signed) arithmetic semantics */
    return 0;
}
