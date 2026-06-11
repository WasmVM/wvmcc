/* LANG-6.5.2.3-04 — Common initial sequence inspection (ISO C17 6.5.2.3p6).
 * If a union contains several structures that share a common initial
 * sequence, and the union object currently contains one of them, it is
 * permitted to inspect the common initial part of any of them, provided
 * the union's declaration is visible. */

union u {
    struct s1 { int tag; int a; }       one;
    struct s2 { int tag; long b; double c; } two;
};

int main(void)
{
    union u v;

    /* Store via one member; read the common initial sequence (`tag`)
     * through the other member's struct type. */
    v.one.tag = 42;
    if (v.two.tag != 42) return 1;

    v.two.tag = 7;
    if (v.one.tag != 7) return 2;

    return 0;
}
