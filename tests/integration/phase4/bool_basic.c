// Phase 4 — _Bool basics: store-side normalization to 0/1.

int test() {
    _Bool b = 5;     // becomes 1 (since 5 != 0)
    _Bool z = 0;     // 0
    b = -7;          // 1
    return (int)b + (int)z;   // 1
}
