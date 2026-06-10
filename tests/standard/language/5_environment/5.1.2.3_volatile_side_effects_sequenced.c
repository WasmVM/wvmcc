/* LANG-5.1.2.3-01 — 5.1.2.3p2/p4: accessing a volatile object is a side
 * effect, and side effects are sequenced according to the abstract machine.
 * Each volatile read must observe the value left by the previously sequenced
 * volatile write. */
volatile int v;

int main(void)
{
    v = 1;
    int a = v;                  /* sequenced after `v = 1` */
    if (a != 1) return 1;

    v = 2;
    int b = v;                  /* sequenced after `v = 2` */
    if (b != 2) return 2;

    v = v + 3;                  /* read then write, sequenced */
    if (v != 5) return 3;

    return 0;
}
