/* LANG-6.4.4.1-03 — constraint violation (C17 6.4.4.1p2,p6): an integer
 * constant whose value is representable by none of the types in its list
 * (and wvmcc has no extended integer types) must be diagnosed. 32 decimal
 * digits exceed ULLONG_MAX; the old lexer silently folded this to 0. */
unsigned long long x = 99999999999999999999999999999999;

int main(void) {
    return 0;
}
