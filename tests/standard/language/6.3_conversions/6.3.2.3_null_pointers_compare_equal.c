/* LANG-6.3.2.3-04 — 6.3.2.3p4: conversion of a null pointer to another pointer
 * type yields a null pointer of that type. Any two null pointers shall compare
 * equal. */

int main(void) {
    int *ip = 0;
    char *cp = 0;
    double *dp = 0;
    void *vp = 0;

    /* Null pointers of differing types compare equal (after the usual
     * conversion to a common type in the comparison). */
    if (ip != (int *)vp) return 1;
    if ((void *)cp != vp) return 2;
    if ((void *)dp != vp) return 3;
    if ((void *)ip != (void *)cp) return 4;
    if ((void *)cp != (void *)dp) return 5;

    /* Converting a null pointer to another pointer type yields a null pointer. */
    char *cp2 = (char *)ip;
    if ((void *)cp2 != (void *)0) return 6;
    if (cp2) return 7;

    return 0;
}
