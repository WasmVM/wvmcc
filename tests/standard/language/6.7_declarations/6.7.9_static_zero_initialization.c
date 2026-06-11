/* LANG-6.7.9-06 — an object with static or thread storage duration that is
 * not initialized explicitly is initialized to zero: arithmetic types to
 * (positive or unsigned) zero, pointers to null, aggregates recursively
 * (C17 6.7.9p10). */
static int si;
static long sl;
static double sd;
static int *sp;
static int sa[4];

struct Z {
    int a;
    long b;
    int *p;
    int arr[3];
};
static struct Z sz;

int tu_scope_no_static; /* external linkage, still static storage duration */

int main(void) {
    if (si != 0) return 1;
    if (sl != 0L) return 2;
    if (sd != 0.0) return 3;
    if (sp != (int *)0) return 4;
    for (int i = 0; i < 4; ++i) {
        if (sa[i] != 0) return 5;
    }
    if (sz.a != 0 || sz.b != 0L) return 6;
    if (sz.p != (int *)0) return 7;
    if (sz.arr[0] != 0 || sz.arr[1] != 0 || sz.arr[2] != 0) return 8;
    if (tu_scope_no_static != 0) return 9;
    return 0;
}
