/* LANG-6.5.6-04 — 6.5.6p8 (ISO C17): when an integer is added to a pointer, the
 * result points to an element offset from the original by that integer count;
 * i.e. the byte advance is (count * sizeof(*ptr)). Verify=exit. */
int main(void) {
    int a[5] = {10, 20, 30, 40, 50};
    int *p = a;

    /* p + 2 points to a[2]. */
    if (*(p + 2) != 30) return 1;

    /* Byte distance equals index distance * sizeof(int). */
    if ((char *)(p + 3) - (char *)p != 3 * (long)sizeof(int)) return 2;

    /* Works for a char* with element size 1. */
    char c[4] = {'a', 'b', 'c', 'd'};
    char *q = c;
    if (*(q + 1) != 'b') return 3;
    if ((q + 1) - q != 1) return 4;

    return 0;
}
