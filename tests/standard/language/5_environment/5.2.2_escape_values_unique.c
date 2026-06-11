/* LANG-5.2.2-02 — 5.2.2p3: each alphabetic escape sequence produces a unique
 * implementation-defined value which can be stored in a single char object.
 * Pairwise-distinctness and char-storability are the standard guarantees;
 * docs/spec.md documents the ASCII values (\a=7 \b=8 \f=12 \n=10 \r=13 \t=9
 * \v=11). */

/* Each escape value is storable in a single char object. */
_Static_assert((char)'\a' == '\a', "\\a fits in a char");
_Static_assert((char)'\b' == '\b', "\\b fits in a char");
_Static_assert((char)'\f' == '\f', "\\f fits in a char");
_Static_assert((char)'\n' == '\n', "\\n fits in a char");
_Static_assert((char)'\r' == '\r', "\\r fits in a char");
_Static_assert((char)'\t' == '\t', "\\t fits in a char");
_Static_assert((char)'\v' == '\v', "\\v fits in a char");

/* Pairwise unique. */
_Static_assert('\a' != '\b' && '\a' != '\f' && '\a' != '\n' && '\a' != '\r'
            && '\a' != '\t' && '\a' != '\v', "\\a is unique");
_Static_assert('\b' != '\f' && '\b' != '\n' && '\b' != '\r' && '\b' != '\t'
            && '\b' != '\v', "\\b is unique");
_Static_assert('\f' != '\n' && '\f' != '\r' && '\f' != '\t' && '\f' != '\v',
               "\\f is unique");
_Static_assert('\n' != '\r' && '\n' != '\t' && '\n' != '\v', "\\n is unique");
_Static_assert('\r' != '\t' && '\r' != '\v', "\\r is unique");
_Static_assert('\t' != '\v', "\\t is unique");
