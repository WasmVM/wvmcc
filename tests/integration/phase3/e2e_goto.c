// Phase 3 e2e — forward goto skipping intervening statements.
int main(void) {
    int s = 5;
    goto end;
    s = 99; // skipped
end:
    if (s != 5) return 1;

    int t = 0;
    goto a;
    t = 100;
a:
    t = t + 10;
    goto b;
    t = t + 99;
b:
    if (t != 10) return 2;

    return 0;
}
