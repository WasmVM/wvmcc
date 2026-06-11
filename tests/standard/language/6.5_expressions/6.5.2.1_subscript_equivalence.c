/* LANG-6.5.2.1-01 — 6.5.2.1p2: the subscript operator E1[E2] is identical to
 * (*((E1)+(E2))), so a[i], i[a], and *(a+i) all denote the same element. */

int main(void)
{
    int a[5] = { 10, 20, 30, 40, 50 };

    /* E1[E2] element access */
    if (a[0] != 10) return 1;
    if (a[3] != 40) return 2;

    /* a[i] == *((a)+(i)) */
    if (a[2] != *((a) + (2))) return 3;

    /* Commutativity: i[a] == a[i] because both are *((a)+(i)). */
    if (2[a] != a[2]) return 4;
    if (4[a] != 50) return 5;

    /* Writing through the subscript form updates the same object. */
    a[1] = 99;
    if (*(a + 1) != 99) return 6;

    return 0;
}
