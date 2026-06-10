/* LANG-6.7.6-01 — declarators (C17 6.7.6p1,p4–p6): pointer, array, and
 * function declarators combine, and parenthesized declarators bind to the
 * innermost identifier first:
 *   int *a[2]    — array of 2 pointers to int
 *   int (*b)[2]  — pointer to array of 2 ints
 *   int (*f)(void) — pointer to function returning int */

static int x = 11, y = 22;
static int arr2[2] = { 33, 44 };

static int forty(void) { return 40; }

int main(void) {
    /* Array of pointers: subscript first, then dereference. */
    int *a[2] = { &x, &y };
    if (*a[0] != 11) return 1;
    if (*a[1] != 22) return 2;

    /* Pointer to array: dereference first, then subscript. */
    int (*b)[2] = &arr2;
    if ((*b)[0] != 33) return 3;
    if ((*b)[1] != 44) return 4;

    /* sizeof distinguishes the two shapes. */
    if (sizeof(a) != 2 * sizeof(int *)) return 5;
    if (sizeof(*b) != 2 * sizeof(int)) return 6;

    /* Pointer to function returning int. */
    int (*f)(void) = forty;
    if (f() != 40) return 7;
    if ((*f)() != 40) return 8;

    return 0;
}
