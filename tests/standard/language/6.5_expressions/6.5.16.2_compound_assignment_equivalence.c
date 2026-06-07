/* LANG-6.5.16.2-01 — A compound assignment `E1 op= E2` behaves as `E1 = E1 op
 * (E2)`, except that the lvalue `E1` is evaluated only once (ISO C17
 * 6.5.16.2p3). */

static int side_effect_count;

static int *select(int *p)
{
    side_effect_count++;
    return p;
}

int main(void)
{
    int a = 10;

    /* Each compound operator behaves as the corresponding `E1 = E1 op (E2)`. */
    a = 10; a += 5;  if (a != 15) return 1;
    a = 10; a -= 3;  if (a != 7)  return 2;
    a = 10; a *= 4;  if (a != 40) return 3;
    a = 20; a /= 6;  if (a != 3)  return 4;
    a = 20; a %= 6;  if (a != 2)  return 5;
    a = 1;  a <<= 4; if (a != 16) return 6;
    a = 64; a >>= 2; if (a != 16) return 7;
    a = 0x0F; a &= 0x36; if (a != 0x06) return 8;
    a = 0x0F; a |= 0x30; if (a != 0x3F) return 9;
    a = 0x0F; a ^= 0x36; if (a != 0x39) return 10;

    /* The RHS is grouped: `a *= b + c` means `a = a * (b + c)`. */
    a = 3; a *= 2 + 4; if (a != 18) return 11;

    /* The lvalue E1 is evaluated only once. */
    {
        int arr[1] = { 100 };
        side_effect_count = 0;
        *select(arr) += 5;
        if (arr[0] != 105) return 12;
        if (side_effect_count != 1) return 13;
    }

    /* Pointer compound assignment: `+=` / `-=` scale by element size. */
    {
        int v[5] = { 0, 1, 2, 3, 4 };
        int *p = v;
        p += 3;
        if (*p != 3) return 14;
        p -= 2;
        if (*p != 1) return 15;
    }

    /* The conversion of the result to the type of E1 is applied, as in simple
     * assignment: a narrowing compound assignment truncates. */
    {
        unsigned char uc = 200;
        uc += 100;                 /* 300 -> stored modulo 256 == 44 */
        if (uc != 44) return 16;
    }

    return 0;
}
