/* tests/standard/libc/string/strcspn.c — LIBC-string-strcspn-01. ISO C17 §7.24.5.3. Verify=exit.
 * strcspn returns the length of the maximal initial segment of s consisting of
 * chars NOT in the reject set. */
#include <string.h>
int main(void) {
    if (strcspn("abcde", "dc") != 2) return 1;  /* stops at 'c' */
    if (strcspn("abcde", "xyz") != 5) return 2; /* none present: full length */
    if (strcspn("abcde", "a") != 0) return 3;   /* first char in set */
    if (strcspn("", "abc") != 0) return 4;
    if (strcspn("abc", "") != 3) return 5;      /* empty set rejects nothing */
    return 0;
}
