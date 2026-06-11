/* LANG-6.2.5-11 — 6.2.5p16: an enumeration comprises a set of named integer
 * constant values; each enumerated type is compatible with char, a signed
 * integer type, or an unsigned integer type, and is a distinct type. */

enum color { RED, GREEN = 5, BLUE };

/* Enumeration constants are integer constant expressions with the declared
 * values (RED=0, GREEN=5, BLUE=6 by the +1 rule of 6.7.2.2p3). */
_Static_assert(RED == 0, "RED is 0");
_Static_assert(GREEN == 5, "explicit GREEN is 5");
_Static_assert(BLUE == 6, "BLUE follows GREEN as 6");

/* The constants are usable in constant expressions (e.g. array bounds, case). */
_Static_assert(BLUE - RED == 6, "enum constants are integer constants");

/* An enumerated type has a fixed object size (compatible with an integer type). */
_Static_assert(sizeof(enum color) >= 1, "enum has object size");
