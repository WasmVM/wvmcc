// Phase 4 e2e — function pointers via call_indirect.
int add(int a, int b) { return a + b; }
int sub(int a, int b) { return a - b; }

int apply(int (*op)(int, int), int x, int y) {
    return op(x, y);
}

int main(void) {
    if (apply(&add, 10, 3) != 13) return 1;
    if (apply(&sub, 10, 3) != 7) return 2;
    int (*fp)(int, int) = &add;
    if (fp(40, 2) != 42) return 3;
    fp = &sub;
    if (fp(40, 2) != 38) return 4;
    return 0;
}
