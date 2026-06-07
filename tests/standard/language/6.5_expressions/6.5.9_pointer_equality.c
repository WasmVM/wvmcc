/* LANG-6.5.9-02 — 6.5.9p5,p6: two pointers compare equal iff both are null,
 * point to the same object (or one past the end of the same array object), or
 * one is one-past-end of one array and the other points to the start of a
 * different array that happens to follow. A null pointer constant compares
 * equal to a pointer (after conversion). Verify=exit. */

int main(void)
{
    int arr[4] = { 0, 1, 2, 3 };
    int *p = &arr[1];
    int *q = &arr[1];
    int *r = &arr[2];

    /* Pointers to the same object compare equal. */
    if ((p == q) != 1) return 1;
    if ((p != q) != 0) return 2;

    /* Pointers to distinct objects compare unequal. */
    if ((p == r) != 0) return 3;
    if ((p != r) != 1) return 4;

    /* A pointer one past the end of an array compares equal to a pointer
     * formed by incrementing through the array. */
    int *end = arr + 4;
    int *walk = &arr[3] + 1;
    if ((end == walk) != 1) return 5;

    /* One-past-end is distinct from the last element. */
    if ((end == &arr[3]) != 0) return 6;

    /* Two null pointers compare equal. */
    int *n1 = 0;
    int *n2 = (void *)0;
    if ((n1 == n2) != 1) return 7;

    /* A null pointer constant compares equal to a null pointer and unequal to
     * a non-null pointer. */
    if ((n1 == 0) != 1) return 8;
    if ((p == 0) != 0) return 9;
    if ((p != 0) != 1) return 10;

    /* void* may be compared against an object pointer (one operand converted
     * to the other's type per 6.5.9p5). */
    void *vp = p;
    if ((vp == p) != 1) return 11;
    if ((vp == r) != 0) return 12;

    return 0;
}
