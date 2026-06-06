/* LANG-6.2.6.1-01 — 6.2.6.1p2,p4: a non-bit-field object occupies
 * n = sizeof(obj) contiguous bytes and can be copied through an
 * unsigned char[n] view (memcpy semantics). Verify=exit.
 *
 * Self-contained: no libc. Copies the object's bytes out and back via an
 * unsigned char array and checks the value round-trips. Returns 0 on
 * success, distinct non-zero on the first failed check. */

struct S { int a; short b; char c; };

static void byte_copy(unsigned char *dst, const unsigned char *src, unsigned long n)
{
    for (unsigned long i = 0; i < n; i++)
        dst[i] = src[i];
}

int main(void)
{
    /* Copy an int through an unsigned char[sizeof(int)] view. */
    int x = 0x12345678;
    unsigned char buf[sizeof(int)];
    byte_copy(buf, (const unsigned char *)&x, sizeof x);
    int y = 0;
    byte_copy((unsigned char *)&y, buf, sizeof y);
    if (y != x)
        return 1;

    /* Same for an aggregate: full-object byte copy must reproduce members. */
    struct S s = { -7, 1234, 'Q' };
    unsigned char sbuf[sizeof(struct S)];
    byte_copy(sbuf, (const unsigned char *)&s, sizeof s);
    struct S t;
    byte_copy((unsigned char *)&t, sbuf, sizeof t);
    if (t.a != s.a)
        return 2;
    if (t.b != s.b)
        return 3;
    if (t.c != s.c)
        return 4;

    return 0;
}
