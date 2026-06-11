/* LANG-6.5.15-02 — If both the second and third operands have arithmetic type,
 * the usual arithmetic conversions are performed to bring them to a common
 * type, and the result has that common type (ISO C17 6.5.15p5). */

int main(void)
{
    /* int and double -> common type is double. */
    if (sizeof(1 ? 2 : 3.0) != sizeof(double)) return 1;
    {
        double d = (1 ? 2 : 3.5);   /* 2 converted to 2.0 */
        if (d != 2.0) return 2;
        double e = (0 ? 2 : 3.5);
        if (e != 3.5) return 3;
    }

    /* int and long -> common type is long. */
    if (sizeof(1 ? (int)0 : (long)0) != sizeof(long)) return 4;

    /* unsigned int and int -> common type is unsigned int; the chosen int
     * operand is converted, so a negative value wraps. */
    {
        unsigned u = (1 ? -1 : 0u);
        if (u != (unsigned)-1) return 5;
    }

    /* char operands undergo integer promotions then UAC; result is int. */
    if (sizeof(1 ? (char)0 : (char)0) != sizeof(int)) return 6;

    return 0;
}
