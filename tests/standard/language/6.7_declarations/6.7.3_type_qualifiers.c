/* LANG-6.7.3-01 — type qualifiers recognized (C17 6.7.3p1,p6):
 * const, volatile, restrict, and _Atomic are valid type qualifiers; if the
 * same qualifier appears more than once in the same specifier-qualifier-list
 * (directly or via typedefs), the behavior is the same as if it appeared
 * just once. */

const int ci = 10;
volatile int vi = 20;
_Atomic int ai = 30;

/* Repeated qualifier counts once (6.7.3p6). */
const const int cci = 40;
typedef const int cint;
const cint tci = 50;          /* const applied twice via typedef */
volatile volatile int vvi = 60;

static int sum_restrict(int *restrict p, int *restrict q) {
    return *p + *q;
}

int main(void) {
    if (ci != 10) return 1;
    if (vi != 20) return 2;
    vi = 21;
    if (vi != 21) return 3;
    if (ai != 30) return 4;
    ai = 31;
    if (ai != 31) return 5;
    if (cci != 40) return 6;
    if (tci != 50) return 7;
    if (vvi != 60) return 8;

    int a = 3, b = 4;
    if (sum_restrict(&a, &b) != 7) return 9;
    return 0;
}
