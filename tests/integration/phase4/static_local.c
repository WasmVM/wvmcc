// Phase 4 — static local variables backed by mem[0] with a one-time init guard.
// Inspect with `readwasm --func --global --data` — should show a guard global
// (mut i32 init 0) and `global.get`/`if` around the initializer.

int next_id() {
    static int id = 0;
    id = id + 1;
    return id;
}

int test() {
    int a = next_id();   // 1
    int b = next_id();   // 2
    int c = next_id();   // 3
    return a + b + c;    // 6
}
