// Phase 4 — designated initializers for structs and arrays.
// Compile with `wvmcc designated_init.c` and verify field offsets and array
// indices in the output via `readwasm --func`.

struct S { int a; int b; int c; };

int test_struct() {
    struct S s = {.b = 2, .a = 1, .c = 3};
    return s.a + s.b + s.c;   // 6
}

int test_array() {
    int arr[3] = {[2] = 9, [0] = 1};
    return arr[0] + arr[2];   // 10
}
