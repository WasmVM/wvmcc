/* LANG-5.2.1-01 — 5.2.1p3: the basic source/execution character sets contain
 * the 52 letters, 10 decimal digits, graphic characters, space and control
 * characters; the value of each character after 0 in the list of decimal
 * digits shall be one greater than the value of the previous (contiguous). */

/* Decimal digits are contiguous in the execution character set. */
_Static_assert('1' - '0' == 1, "digit 1 follows 0");
_Static_assert('2' - '0' == 2, "digit 2 contiguous");
_Static_assert('3' - '0' == 3, "digit 3 contiguous");
_Static_assert('4' - '0' == 4, "digit 4 contiguous");
_Static_assert('5' - '0' == 5, "digit 5 contiguous");
_Static_assert('6' - '0' == 6, "digit 6 contiguous");
_Static_assert('7' - '0' == 7, "digit 7 contiguous");
_Static_assert('8' - '0' == 8, "digit 8 contiguous");
_Static_assert('9' - '0' == 9, "digit 9 contiguous");

/* Required members are present and distinct (usable in constant expressions):
 * upper-case letters, lower-case letters, space, and graphic characters. */
_Static_assert('A' != 'a' && 'Z' != 'z', "upper and lower case letters are distinct members");
_Static_assert(' ' != '0' && ' ' != 'A', "space character is a distinct member");
_Static_assert('!' != '"' && '#' != '%' && '&' != '\'' && '(' != ')',
               "graphic characters are distinct members");
_Static_assert('*' != '+' && ',' != '-' && '.' != '/' && ':' != ';',
               "graphic characters are distinct members (2)");
_Static_assert('<' != '=' && '>' != '?' && '[' != '\\' && ']' != '^',
               "graphic characters are distinct members (3)");
_Static_assert('_' != '{' && '|' != '}' && '~' != '!',
               "graphic characters are distinct members (4)");
