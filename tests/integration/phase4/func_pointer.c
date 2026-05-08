// Phase 4 — function pointers and call_indirect.
// Compile with `wvmcc func_pointer.c -o out.wasm` and inspect with
// `readwasm --func --table --type out.wasm`. Should show a funcref table,
// an active element segment, and a `call_indirect` in `apply`.

int add(int a, int b) { return a + b; }
int sub(int a, int b) { return a - b; }

int apply(int (*op)(int, int), int x, int y) {
    return op(x, y);
}

int test() {
    int (*fp)(int, int) = &add;
    int s = fp(10, 3);     // 13
    fp = &sub;
    s = s + fp(10, 3);     // 13 + 7 = 20
    s = s + apply(&add, 100, 1);  // 20 + 101 = 121
    return s;
}
