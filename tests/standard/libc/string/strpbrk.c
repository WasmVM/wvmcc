/* tests/standard/libc/string/strpbrk.c — LIBC-string-strpbrk-01. ISO C17 §7.24.5.4. Verify=exit.
 * strpbrk returns a pointer to the first char of s that is in the accept set,
 * or NULL if none is. */
#include <string.h>
int main(void) {
    const char *s = "abcde";
    if (strpbrk(s, "dc") != s + 2) return 1; /* 'c' comes first */
    if (strpbrk(s, "xyz") != NULL) return 2;
    if (strpbrk(s, "a") != s) return 3;
    if (strpbrk(s, "") != NULL) return 4;    /* empty set matches nothing */
    if (strpbrk("", "abc") != NULL) return 5;
    return 0;
}
