/* tests/standard/libc/string/strerror.c — LIBC-string-strerror-01. ISO C17 §7.24.6.2. Verify=exit.
 * strerror maps an error number to a pointer to a (non-null, null-terminated)
 * message string. The content is implementation-defined; only check shape. */
#include <string.h>
#include <errno.h>
int main(void) {
    char *m = strerror(0);
    if (m == NULL) return 1;
    char *e = strerror(EDOM);
    if (e == NULL) return 2;
    if (strlen(e) == 0) return 3;  /* a message, not an empty string */
    char *r = strerror(ERANGE);
    if (r == NULL) return 4;
    return 0;
}
