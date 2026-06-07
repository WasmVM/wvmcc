/* LANG-6.5.16-01 — An assignment operator stores a value in the object
 * designated by the left operand. The value of an assignment expression is the
 * value of the left operand after the assignment, but it is not an lvalue
 * (ISO C17 6.5.16p3). */

int main(void)
{
    int a;

    /* The assignment stores into `a`. */
    a = 42;
    if (a != 42) return 1;

    /* The value of the assignment expression is the LHS after the store. */
    if ((a = 7) != 7) return 2;
    if (a != 7) return 3;

    /* The result is a value, not an lvalue; conversions (e.g. truncation)
     * applied by the assignment are reflected in the expression's value. */
    {
        char c;
        int v = (c = 0x141);          /* stored value is truncated to char */
        if (c != (char)0x141) return 4;
        if (v != (char)0x141) return 5;  /* expression value == stored value */
    }

    /* The result not being an lvalue means it cannot itself be assigned to,
     * but it can be used as an operand: chained assignment is right-associative
     * and uses the (converted) value of the inner assignment. */
    {
        int x, y;
        x = y = 9;
        if (x != 9) return 6;
        if (y != 9) return 7;
    }

    return 0;
}
