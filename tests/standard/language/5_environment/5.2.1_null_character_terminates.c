/* LANG-5.2.1-03 — 5.2.1p2: a byte with all bits set to 0, called the null
 * character, shall exist in the basic execution character set; it is used to
 * terminate a character string.  '\0' == 0, and sizeof a string literal
 * counts the implicit terminating null. */

_Static_assert('\0' == 0, "null character has value 0");
_Static_assert(sizeof "" == 1, "empty string literal is exactly the terminating null");
_Static_assert(sizeof "abc" == 4, "string literal includes the terminating null");
_Static_assert(sizeof "\0" == 2, "embedded null escape occupies one byte plus terminator");
