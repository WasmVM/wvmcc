/* LANG-6.5.16.2-01 — A compound assignment `E1 op= E2` behaves as the simple
 * assignment `E1 = E1 op (E2)`, except that the lvalue E1 is evaluated only
 * once (ISO C17 6.5.16.2p3). */

static int calls;

static int *select(int *p)
{
    ++calls;
    return p;
}

int main(void)
{
    /* Arithmetic compound assignment computes E1 op (E2) and stores it. */
    {
        int x = 10;
        x += 5;
        if (x != 15) return 1;
        x -= 3;
        if (x != 12) return 2;
        x *= 2;
        if (x != 24) return 3;
        x /= 5;
        if (x != 4) return 4;
        x %= 3;
        if (x != 1) return 5;
        x <<= 4;
        if (x != 16) return 6;
        x >>= 2;
        if (x != 4) return 7;
        x &= 6;
        if (x != 4) return 8;
        x |= 1;
        if (x != 5) return 9;
        x ^= 7;
        if (x != 2) return 10;
    }

    /* The lvalue designating the target is evaluated exactly once. */
    {
        int arr[2] = {100, 0};
        calls = 0;
        *select(arr) += 1;   /* select() must be called only once */
        if (calls != 1) return 11;
        if (arr[0] != 101) return 12;
    }

    /* `+=` on a pointer with integer RHS advances by element size. */
    {
        int a[4] = {0, 1, 2, 3};
        int *p = a;
        p += 2;
        if (*p != 2) return 13;
        p -= 1;
        if (*p != 1) return 14;
    }

    return 0;
}
