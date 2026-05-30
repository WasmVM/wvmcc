// M2 #77 — file-scope (global) variable codegen.
// Exercises: scalar globals with initializers, zero-init globals, an array
// global, a struct global, and cross-function mutation. All storage lives in
// mem[0] (GlobalMem); reads/writes go through i64.const address + load/store.
//
// NOTE: avoids `i64 != / ==` inside `if`+`return` (a known WasmVM interpreter
// trap, out of scope) by using `int` (i32) comparisons and a compute-and-
// return accumulator.

int g_init = 7;          // initialized scalar (data segment)
int g_zero;              // zero-init scalar (no data segment, memory starts 0)
int g_arr[4] = {1, 2, 3, 4};
struct Pt { int x; int y; } g_pt = {10, 20};

void bump(void) {
    g_zero = g_zero + 5;     // mutate a zero-init global across a call
    g_arr[2] = g_arr[2] + 6; // array-element store into a global
}

int main(void) {
    int acc = 0;
    acc = acc + g_init;          // 7
    bump();
    acc = acc + g_zero;          // +5  -> 12
    acc = acc + g_arr[0];        // +1  -> 13
    acc = acc + g_arr[2];        // +(3+6)=9 -> 22
    acc = acc + g_pt.x;          // +10 -> 32
    acc = acc + g_pt.y;          // +20 -> 52
    // Expected total: 52. Return 0 on success so wasmvm exit code == 0.
    return acc - 52;
}
