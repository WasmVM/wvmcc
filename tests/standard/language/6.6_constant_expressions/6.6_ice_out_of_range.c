/* LANG-6.6-03 — Constraint: each constant expression shall evaluate to a
 * constant that is in the range of representable values for its type
 * (ISO C17 6.6p4). 2147483647 is INT_MAX on this implementation (32-bit
 * int); adding 1 in an int-typed constant expression produces a value
 * outside the range of int, which a conforming compiler must reject. */

enum e { E = 2147483647 + 1 };  /* ill-formed: int overflow in a constant expression */
