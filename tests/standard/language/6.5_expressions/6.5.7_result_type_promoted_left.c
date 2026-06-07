/* LANG-6.5.7-07 — 6.5.7p3 (ISO C17): the integer promotions are performed on
 * each operand. The type of the result is that of the promoted left operand.
 * The usual arithmetic conversions are NOT performed: the right operand's type
 * does not affect the result type.
 * Verify=exit: return 0 on success, a distinct non-zero code per failed check. */
int main(void) {
    /* Left operand `unsigned char` promotes to (signed) int; result type is
     * int, so the result behaves as a signed int. */
    unsigned char uc = 0x01;
    if (sizeof(uc << 1) != sizeof(int)) return 1;

    /* short promotes to int as well */
    short s = 1;
    if (sizeof(s << 1) != sizeof(int)) return 2;

    /* The right operand type is irrelevant to the result type: shifting an int
     * by a long long count still yields an int-typed result. */
    int i = 1;
    long long cnt = 3;
    if (sizeof(i << cnt) != sizeof(int)) return 3;

    /* Left operand unsigned long long keeps its (promoted) type regardless of
     * a plain-int right operand -> no usual arithmetic conversions narrowing. */
    unsigned long long ull = 1ull;
    if (sizeof(ull << 1) != sizeof(unsigned long long)) return 4;

    /* Value check: promoting unsigned char 0xFF to int and shifting left 4
     * gives 0xFF0 == 4080, representable in int (no wraparound to a small
     * unsigned-char modulus). */
    unsigned char hi = 0xFF;
    if ((hi << 4) != 0xFF0) return 5;

    return 0;
}
