/* LANG-6.7.9-12 — if there are fewer initializers in a brace-enclosed list
 * than there are elements/members of an aggregate, the remainder is
 * initialized implicitly the same as objects that have static storage
 * duration, i.e. to zero (C17 6.7.9p21). */
struct S {
    int x;
    long y;
    int *p;
    int arr[3];
};

int main(void) {
    int a[5] = {1, 2}; /* a[2..4] zero-filled */
    if (a[0] != 1 || a[1] != 2) return 1;
    if (a[2] != 0 || a[3] != 0 || a[4] != 0) return 2;

    struct S s = {4}; /* y, p, arr zero-filled */
    if (s.x != 4) return 3;
    if (s.y != 0L) return 4;
    if (s.p != (int *)0) return 5;
    if (s.arr[0] != 0 || s.arr[1] != 0 || s.arr[2] != 0) return 6;

    int empty_tail[2][2] = {{1}}; /* whole second row zero-filled */
    if (empty_tail[0][0] != 1 || empty_tail[0][1] != 0) return 7;
    if (empty_tail[1][0] != 0 || empty_tail[1][1] != 0) return 8;

    return 0;
}
