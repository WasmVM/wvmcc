/* LANG-6.3.1.1-03 — 6.3.1.1p3: whether a "plain" char is treated as signed or
 * unsigned is implementation-defined, which determines whether a plain char can
 * hold (and promote to) negative values. wvmcc documents plain char as signed
 * (docs/spec.md), so (char)-1 is negative and CHAR_MIN < 0.
 *
 * The standard guarantees, independent of the implementation choice, that plain
 * char has the same range/representation as either signed char or unsigned
 * char; this test asserts the documented signed-char behavior. */

/* In the documented (signed) implementation, the bit pattern 0xFF stored in a
 * char represents the value -1, so promotion to int yields a negative value. */
_Static_assert((char)0xFF < 0, "plain char is signed: (char)0xFF promotes to -1");
_Static_assert((char)-1 == -1, "plain char preserves -1");
_Static_assert((char)-1 != (unsigned char)-1,
               "signed plain char differs from unsigned char for 0xFF pattern");

/* The smallest value representable in a (signed) plain char is negative. */
_Static_assert(((char)-128) < 0, "char can hold a negative minimum");

int main(void) { return 0; }
