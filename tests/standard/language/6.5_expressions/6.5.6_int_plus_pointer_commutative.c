/* LANG-6.5.6-05 — 6.5.6p8 (ISO C17): addition of a pointer and an integer is
 * commutative; `int + ptr` yields the same result as `ptr + int`. Verify=exit. */
int main(void) {
    int a[5] = {10, 20, 30, 40, 50};
    int *p = a;

    if ((2 + p) != (p + 2)) return 1;
    if (*(2 + p) != 30) return 2;
    if (*(3 + p) != *(p + 3)) return 3;

    return 0;
}
