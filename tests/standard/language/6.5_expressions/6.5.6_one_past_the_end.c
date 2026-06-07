/* LANG-6.5.6-07 — 6.5.6p8,p7 (ISO C17): a pointer to one past the last element
 * of an array is a valid pointer value that may be computed and compared,
 * provided it is not dereferenced. (p7: a non-array object is treated as an
 * array of length one for this purpose.) Verify=exit. */
int main(void) {
    int a[3] = {1, 2, 3};
    int *end = a + 3;          /* one past the end: valid to compute */

    if (!(end > a)) return 1;            /* comparable */
    if ((end - a) != 3) return 2;        /* difference is the length */
    if (end != &a[3]) return 3;          /* &a[3] designates one-past-the-end */

    /* p7: a scalar object acts as an array of length one. */
    int x = 42;
    int *xp = &x;
    int *xend = xp + 1;
    if (xend == xp) return 4;
    if ((xend - xp) != 1) return 5;

    return 0;
}
