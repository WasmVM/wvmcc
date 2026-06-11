/* LANG-6.2.5-03 — 6.2.5p3: whether a char whose value is not a basic-set member
 * is treated as signed or unsigned is implementation-defined.
 * docs/spec.md fixes wvmcc's choice: plain char is signed. */

/* With a signed plain char, a byte pattern with the high bit set (e.g. 0xFF)
 * round-trips to a negative value, exactly as for signed char. */
_Static_assert((char)0xFF < 0, "plain char is signed (high-bit value is negative)");
_Static_assert((char)0xFF == (signed char)0xFF, "plain char matches signed char");
_Static_assert((char)-1 == -1, "signed char round-trips -1");
