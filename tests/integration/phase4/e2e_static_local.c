// Phase 4 e2e — static local variables persist across calls.
int next_id(void) {
    static int id = 0;
    id = id + 1;
    return id;
}

int main(void) {
    int a = next_id();    // 1
    int b = next_id();    // 2
    int c = next_id();    // 3
    if (a != 1) return 1;
    if (b != 2) return 2;
    if (c != 3) return 3;
    if (a + b + c != 6) return 4;
    return 0;
}
