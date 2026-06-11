/* LANG-6.7.9-13 — an array of unknown size is completed by its initializer:
 * its size is the largest indexed element plus one, with designators counted
 * (C17 6.7.9p22). */
int a[] = {[0] = 1, [7] = 8, [3] = 4}; /* largest index 7 -> 8 elements */
int b[] = {1, 2, 3};                   /* 3 elements */

_Static_assert(sizeof(a) / sizeof(a[0]) == 8, "sized by largest designated index");
_Static_assert(sizeof(b) / sizeof(b[0]) == 3, "sized by initializer count");

int main(void) {
    if (a[0] != 1 || a[3] != 4 || a[7] != 8) return 1;
    if (a[1] != 0 || a[2] != 0 || a[4] != 0 || a[5] != 0 || a[6] != 0) return 2;
    if (b[0] != 1 || b[1] != 2 || b[2] != 3) return 3;
    return 0;
}
