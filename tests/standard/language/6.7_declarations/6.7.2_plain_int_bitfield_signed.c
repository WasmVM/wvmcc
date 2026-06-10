/* LANG-6.7.2-04 — 6.7.2p5: for bit-fields, whether a plain `int` bit-field is
 * treated as `signed int` or `unsigned int` is implementation-defined.
 * wvmcc documents (docs/spec.md) that a plain `int` bit-field is SIGNED.
 * The bit-field's signedness is probed at translation time with _Generic on an
 * unevaluated member access: a signed bit-field selects the `int` association,
 * an unsigned one would select `unsigned int`. */

struct probe {
    int bf : 4; /* plain int bit-field: documented as signed */
};

_Static_assert(_Generic(((struct probe *)0)->bf,
                        int: 1,
                        unsigned int: 0,
                        default: 0),
               "plain `int` bit-field is signed (documented behavior)");
