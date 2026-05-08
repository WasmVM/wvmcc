// Phase 3 e2e — switch statement with default, break, and fallthrough.
//
// All branches assign to a single result variable rather than using `return`
// inside the switch cases. wvmcc's lowering of `return` inside a nested
// `if`/`switch` body inside a non-main function produces wasm that wasmvm
// crashes on at runtime (separate compiler bug).
int dispatch(int x) {
    int r = -1;
    switch (x) {
        case 0: r = 100; break;
        case 1: r = 200; break;
        case 5: r = 500; break;
        default: r = -1; break;
    }
    return r;
}

int with_default(int x1) {
    int r1 = 0;
    switch (x1) {
        case 1: r1 = 10; break;
        case 2: r1 = 20; break;
        default: r1 = 99;
    }
    return r1;
}

int fallthrough(int x2) {
    int r2 = 0;
    switch (x2) {
        case 1: r2 = r2 + 1; /* fallthrough */
        case 2: r2 = r2 + 2; break;
        case 3: r2 = r2 + 3; break;
    }
    return r2;
}

int main(void) {
    if (dispatch(0) != 100) return 1;
    if (dispatch(5) != 500) return 2;
    if (dispatch(99) != -1) return 3;
    if (with_default(1) != 10) return 4;
    if (with_default(7) != 99) return 5;
    if (fallthrough(1) != 3) return 6; // 1 then 2 → r=3
    if (fallthrough(2) != 2) return 7;
    if (fallthrough(3) != 3) return 8;
    return 0;
}
