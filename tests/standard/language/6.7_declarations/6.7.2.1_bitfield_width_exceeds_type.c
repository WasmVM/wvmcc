/* LANG-6.7.2.1-05 — constraint violation (C17 6.7.2.1p4): the width of a
 * bit-field shall not exceed the width of an object of the declared type
 * (unsigned int is 32 bits wide on this target; 40 exceeds it).
 * A conforming compiler must reject this TU. */
struct s {
    unsigned int f : 40; /* error: width exceeds the width of unsigned int */
};
