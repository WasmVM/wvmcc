/* LANG-6.3.1.8-02 — 6.3.1.8p1: if both operands have signed (or both unsigned)
 * integer types, the operand of lesser integer conversion rank is converted to
 * the type of the operand with the greater rank. */

int main(void) {
    /* int + long -> long. The int operand becomes long; no truncation. */
    int i = -1;
    long l = 1L;
    long r = i + l;        /* (-1) + 1 == 0 in long */
    if (r != 0L) return 1;

    /* Sum exceeding INT_MAX is fine because the common type is long (LP64). */
    int big_int = 2000000000;
    long bigger = 2000000000L;
    long sum = big_int + bigger;     /* both converted to long; 4e9 fits long */
    if (sum != 4000000000L) return 2;

    /* unsigned int + unsigned long -> unsigned long. */
    unsigned int ui = 4000000000u;
    unsigned long ul = 4000000000ul;
    unsigned long usum = ui + ul;    /* 8e9 fits unsigned long */
    if (usum != 8000000000ul) return 3;

    /* short + int -> int (short promotes first, then ranks compared). */
    short s = 100;
    int n = 200;
    if ((s + n) != 300) return 4;

    return 0;
}
