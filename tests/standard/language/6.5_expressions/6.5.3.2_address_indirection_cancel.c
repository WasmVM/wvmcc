/* LANG-6.5.3.2-03 — 6.5.3.2p3: `&*E` is equivalent to `E` (the `&` and `*`
 * cancel, neither being evaluated), and `&E1[E2]` is equivalent to
 * `((E1)+(E2))`. */

int main(void)
{
    int a[5] = { 10, 20, 30, 40, 50 };
    int *e = a + 2;

    /* &*E ≡ E : the address of the dereference is the original pointer. */
    if (&*e != e) return 1;
    if (&*e != a + 2) return 2;

    /* &E1[E2] ≡ (E1)+(E2) */
    if (&a[3] != (a + 3)) return 3;
    if (&a[0] != a) return 4;

    /* The cancelled forms still designate the right object when used. */
    *(&a[1]) = 99;
    if (a[1] != 99) return 5;

    /* &E1[E2] commutes the same way E1[E2] does. */
    if (&2[a] != &a[2]) return 6;

    return 0;
}
