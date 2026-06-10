/* tests/standard/libc/stddef/null.c — LIBC-stddef-NULL-01 (C17 7.19p3).
 * Verify=exit. NULL expands to an implementation-defined null pointer
 * constant; a null pointer compares unequal to a pointer to any object
 * and equal to any other null pointer (6.3.2.3). */
#include <stddef.h>

int g;

int main(void) {
    void *p = NULL;
    if (p) return 1;                /* null pointer is "false" */
    char *q = NULL;
    if (q != p) return 2;           /* two null pointers compare equal */
    if (NULL != 0) return 3;        /* NULL is a null pointer constant */
    int *gp = &g;
    if (gp == NULL) return 4;       /* object pointer != null pointer */
    return 0;
}
