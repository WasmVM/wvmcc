/* LANG-6.3.1.8-01 — 6.3.1.8p1: in the usual arithmetic conversions, if either
 * operand has a floating type (long double / double / float), the other operand
 * is converted to that floating type and the result has that type. */

int main(void) {
    /* int + double -> double: result carries the fractional part. */
    int i = 1;
    double d = 0.5;
    if (i + d != 1.5) return 1;

    /* int + float -> float (still exactly representable here). */
    int j = 3;
    float f = 0.25f;
    if (j + f != 3.25f) return 2;

    /* An integer too large for float-but-fine-for-double promotes to double. */
    long big = 9007199254740993L; /* 2^53 + 1, exact in double after no rounding */
    double dd = 0.0;
    /* big converts to double; adding 0.0 keeps the double value. */
    if ((big + dd) - 9007199254740992.0 != 1.0 && (big + dd) != 9007199254740992.0)
        return 3;

    /* Division mixing int and double yields a non-integral double. */
    int a = 7;
    if (a / 2.0 != 3.5) return 4;

    /* float operand forces float arithmetic on an int operand. */
    int n = 10;
    float half = 0.5f;
    if (n * half != 5.0f) return 5;

    return 0;
}
