/* LANG-6.5.6-06 — 6.5.6p8 (ISO C17): subtracting an integer from a pointer
 * yields a pointer that points to an element offset backward by that integer
 * count; the byte retreat is (count * sizeof(*ptr)). Verify=exit. */
int main(void) {
    int a[5] = {10, 20, 30, 40, 50};
    int *p = &a[4];

    if (*(p - 2) != 30) return 1;
    if ((char *)p - (char *)(p - 3) != 3 * (long)sizeof(int)) return 2;
    if (*(p - 4) != 10) return 3;

    return 0;
}
