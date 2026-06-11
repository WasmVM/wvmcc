/* tests/standard/libc/string/strspn.c — LIBC-string-strspn-01. ISO C17 §7.24.5.6. Verify=exit.
 * strspn returns the length of the maximal initial segment of s consisting
 * entirely of chars from the accept set. */
#include <string.h>
int main(void) {
    if (strspn("aabbc", "ab") != 4) return 1;
    if (strspn("abcde", "xyz") != 0) return 2;
    if (strspn("aaa", "a") != 3) return 3;    /* whole string in set */
    if (strspn("", "abc") != 0) return 4;
    if (strspn("abc", "") != 0) return 5;     /* empty set accepts nothing */
    return 0;
}
