/* LANG-6.5.8-01 — 6.5.8p6: each of the operators < (less than), > (greater
 * than), <= (less than or equal to) and >= (greater than or equal to) shall
 * yield 1 if the specified relation is true and 0 if it is false; the result
 * has type int. Verified for arithmetic operands. */

int main(void)
{
    int a = 3, b = 5;

    /* True relations yield 1, false yield 0. */
    if ((a < b) != 1) return 1;
    if ((b < a) != 0) return 2;
    if ((b > a) != 1) return 3;
    if ((a > b) != 0) return 4;
    if ((a <= a) != 1) return 5;
    if ((b <= a) != 0) return 6;
    if ((a >= a) != 1) return 7;
    if ((a >= b) != 0) return 8;

    /* The result type is int (size of an int, not the operand type). */
    if (sizeof(a < b) != sizeof(int)) return 9;

    /* Works with mixed/floating arithmetic operands via usual conversions. */
    if ((2.5 < 3.0) != 1) return 10;
    if ((3.0 <= 2.5) != 0) return 11;

    return 0;
}
