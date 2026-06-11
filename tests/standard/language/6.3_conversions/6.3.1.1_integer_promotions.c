/* LANG-6.3.1.1-01 — 6.3.1.1p2: the integer promotions. An object or expression
 * of a type whose integer conversion rank is less than or equal to the rank of
 * int and unsigned int is converted to int (if int can represent all values of
 * the original type) or to unsigned int. char, short, _Bool, and bit-fields are
 * promoted to int (or unsigned int), and the value is preserved. */

struct bits {
    unsigned int field : 3;   /* range 0..7, fits in int -> promotes to int */
};

int main(void) {
    /* char promotes to int, value preserved. */
    char c = 100;
    if (c + 0 != 100) return 1;

    /* short promotes to int, value preserved (including negative). */
    short s = -1234;
    if (s + 0 != -1234) return 2;

    /* unsigned short: all values representable in int (int is wider here),
     * so it promotes to int. */
    unsigned short us = 60000u;
    if (us + 0 != 60000) return 3;

    /* _Bool promotes to int (0 or 1). */
    _Bool b = 1;
    if (b + 0 != 1) return 4;
    _Bool b0 = 0;
    if (b0 + 0 != 0) return 5;

    /* signed char with a negative value: value preserved after promotion. */
    signed char sc = -42;
    if (sc + 0 != -42) return 6;

    /* A bit-field whose declared range fits in int promotes to int. */
    struct bits x;
    x.field = 5u;
    if (x.field + 0 != 5) return 7;

    /* Promotion produces an int: applying unary minus to a small unsigned
     * short of value 1 yields int -1, not a large unsigned value. */
    unsigned short one = 1u;
    if (-one != -1) return 8;

    return 0;
}
