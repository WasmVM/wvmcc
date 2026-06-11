/* LANG-5.2.1-02 — 5.2.1p1: the values of the members of the execution
 * character set are implementation-defined.  docs/spec.md documents the
 * execution character set as UTF-8 (ASCII-compatible): 'A'==65 etc. */

_Static_assert('A' == 65, "execution charset is ASCII: 'A' == 65");
_Static_assert('Z' == 90, "execution charset is ASCII: 'Z' == 90");
_Static_assert('a' == 97, "execution charset is ASCII: 'a' == 97");
_Static_assert('z' == 122, "execution charset is ASCII: 'z' == 122");
_Static_assert('0' == 48, "execution charset is ASCII: '0' == 48");
_Static_assert('9' == 57, "execution charset is ASCII: '9' == 57");
_Static_assert(' ' == 32, "execution charset is ASCII: space == 32");
_Static_assert('+' == 43, "execution charset is ASCII: '+' == 43");
_Static_assert('~' == 126, "execution charset is ASCII: '~' == 126");
