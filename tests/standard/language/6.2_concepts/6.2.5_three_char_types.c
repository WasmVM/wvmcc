/* LANG-6.2.5-10 — 6.2.5p15: char, signed char, and unsigned char are three
 * distinct types (collectively the character types); plain char behaves the same
 * as one of the other two. docs/spec.md: plain char behaves as signed char. */

/* All three character types share the same width. */
_Static_assert(sizeof(char) == 1, "char is 1 byte");
_Static_assert(sizeof(signed char) == 1, "signed char is 1 byte");
_Static_assert(sizeof(unsigned char) == 1, "unsigned char is 1 byte");

/* unsigned char is unsigned: 0xFF is positive. signed char is signed: negative.
 * These differing sign behaviors witness that the types are distinct. */
_Static_assert((unsigned char)0xFF == 255, "unsigned char holds 255");
_Static_assert((signed char)0xFF == -1, "signed char holds -1 for 0xFF");

/* Plain char behaves as signed char (wvmcc's implementation-defined choice). */
_Static_assert((char)0xFF == (signed char)0xFF, "plain char behaves as signed char");
