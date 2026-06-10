/* LANG-6.7.2.1-06 — constraint violation (C17 6.7.2.1p5): a bit-field shall
 * have a type that is a qualified or unqualified version of _Bool, signed
 * int, unsigned int, or some other implementation-defined type. A floating
 * type is never permitted; a conforming compiler must reject this TU. */
struct s {
    float f : 4; /* error: bit-field of floating type */
};
