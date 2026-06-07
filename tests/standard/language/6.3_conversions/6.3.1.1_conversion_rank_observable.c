/* LANG-6.3.1.1-02 — 6.3.1.1p1/p2: integer conversion rank ordering is
 * observable. Operands of type char have rank less than int, so in an
 * arithmetic expression each is promoted to int and the operation is performed
 * in int, not in char. */

int main(void) {
    /* char + char is evaluated as int: the sum 100+100 = 200 does NOT overflow
     * or wrap to a char-sized result; it is a plain int 200. */
    char a = 100;
    char b = 100;
    if (a + b != 200) return 1;

    /* sizeof(a + b) is sizeof(int): the result type of the promoted addition is
     * int regardless of the operand types. */
    if (sizeof(a + b) != sizeof(int)) return 2;

    /* Two signed chars whose int-sum is negative: still computed in int. */
    char n1 = -100;
    char n2 = -100;
    if (n1 + n2 != -200) return 3;

    /* short has lower rank than int as well: short*short done in int. */
    short s1 = 300;
    short s2 = 300;
    if (s1 * s2 != 90000) return 4;
    if (sizeof(s1 * s2) != sizeof(int)) return 5;

    /* A single promoted char operand: sizeof(+a) is sizeof(int). */
    if (sizeof(+a) != sizeof(int)) return 6;

    return 0;
}
