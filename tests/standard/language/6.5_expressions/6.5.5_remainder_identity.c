/* LANG-6.5.5-02 — The binary `%` operator yields the remainder of the integer
 * division of its operands, and the result of `a/b` and `a%b` satisfies the
 * identity `(a/b)*b + a%b == a` whenever a/b is representable
 * (ISO C17 6.5.5p5,p6). */

static int check(int a, int b)
{
    /* Fundamental identity from 6.5.5p6. */
    return ((a / b) * b + a % b) == a;
}

int main(void)
{
    /* Basic remainder. */
    if ((7 % 3) != 1) return 1;

    /* Exact division -> zero remainder. */
    if ((6 % 3) != 0) return 2;

    /* Because `/` truncates toward zero, the remainder takes the sign of the
     * dividend: -7/3 == -2, so -7 % 3 == -1. */
    if ((-7 % 3) != -1) return 3;

    /* Positive dividend, negative divisor: 7/-3 == -2, so 7 % -3 == 1. */
    if ((7 % -3) != 1) return 4;

    /* Both negative: -7/-3 == 2, so -7 % -3 == -1. */
    if ((-7 % -3) != -1) return 5;

    /* The identity must hold across a spread of sign combinations. */
    if (!check(7, 3)) return 6;
    if (!check(-7, 3)) return 7;
    if (!check(7, -3)) return 8;
    if (!check(-7, -3)) return 9;
    if (!check(100, 7)) return 10;
    if (!check(-100, 7)) return 11;

    return 0;
}
