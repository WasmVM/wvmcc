/* tests/standard/libc/string/strstr.c — LIBC-string-strstr-01. ISO C17 §7.24.5.7. Verify=exit.
 * strstr finds the first occurrence of needle in haystack; an empty needle
 * matches at the start. Returns NULL if absent. */
#include <string.h>
int main(void) {
    const char *s = "ababc";
    if (strstr(s, "abc") != s + 2) return 1;  /* first full match */
    if (strstr(s, "ab") != s) return 2;
    if (strstr(s, "xyz") != NULL) return 3;
    if (strstr(s, "") != s) return 4;         /* empty needle -> haystack */
    if (strstr(s, "ababcd") != NULL) return 5; /* needle longer than haystack */
    return 0;
}
