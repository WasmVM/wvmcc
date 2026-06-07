/* LANG-6.5.9-01 — 6.5.9p3: the == (equal to) and != (not equal to) operators
 * shall yield 1 if the specified relation is true and 0 if it is false; the
 * result has type int. Verify=exit. */

int main(void)
{
    int a = 3, b = 5;

    /* True/false relations yield 1/0. */
    if ((a == a) != 1) return 1;
    if ((a == b) != 0) return 2;
    if ((a != b) != 1) return 3;
    if ((a != a) != 0) return 4;

    /* The result type is int. */
    if (sizeof(a == b) != sizeof(int)) return 5;
    if (sizeof(a != b) != sizeof(int)) return 6;

    /* Works with floating arithmetic operands via usual conversions. */
    if ((2.5 == 2.5) != 1) return 7;
    if ((2.5 != 3.0) != 1) return 8;

    /* Mixed integer/floating operands. */
    if ((3 == 3.0) != 1) return 9;
    if ((3 != 3.0) != 0) return 10;

    return 0;
}
