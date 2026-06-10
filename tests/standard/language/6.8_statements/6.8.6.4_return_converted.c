/* LANG-6.8.6.4-01 — If a `return` statement with an expression is executed,
 * the value of the expression is returned to the caller, converted to the
 * return type of the function as if by assignment to an object of that type
 * (ISO C17 6.8.6.4p3). */

static int return_double_as_int(void)
{
    return 3.75;            /* converted as by assignment: truncates to 3 */
}

static unsigned char return_wrapped(void)
{
    return 260;             /* converted to unsigned char: 260 % 256 = 4 */
}

static long return_int_as_long(void)
{
    int v = -7;
    return v;               /* int -> long, value preserved */
}

static double return_int_as_double(void)
{
    return 5;               /* int -> double, exact */
}

int main(void)
{
    if (return_double_as_int() != 3) return 1;
    if (return_wrapped() != 4) return 2;
    if (return_int_as_long() != -7L) return 3;
    if (return_int_as_double() != 5.0) return 4;
    return 0;
}
