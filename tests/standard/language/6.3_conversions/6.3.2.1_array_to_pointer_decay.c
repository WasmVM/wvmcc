/* LANG-6.3.2.1-03 — 6.3.2.1p3: array-to-pointer decay. Except when it is the
 * operand of sizeof, the unary & operator, or is a string literal used to
 * initialize an array, an expression of type "array of T" is converted to an
 * expression of type "pointer to T" that points to the first element of the
 * array object and is not an lvalue. */

/* The member-decay checks below use file-scope objects on purpose. A *local*
 * struct whose array member has a brace initializer is a separate, unrelated
 * wvmcc defect (the nested element stores are not emitted), and this row is
 * about 6.3.2.1 decay, not about initialization. */
struct S { unsigned int len; char buf[8]; };
struct Outer { int pad; struct S inner; };

struct S     g_s = { 3, { 'a', 'b', 'c' } };
struct Outer g_o = { 0, { 2, { 'x', 'y' } } };
char         g_m[2][4] = { { 'p', 'q' }, { 'r', 's' } };

int main(void) {
    int arr[4] = { 10, 20, 30, 40 };

    /* In a value context, `arr` decays to &arr[0]. */
    int *p = arr;
    if (p != &arr[0]) return 1;
    if (*p != 10) return 2;
    if (p[2] != 30) return 3;

    /* Subscripting works via the decayed pointer; arr[i] == *(arr + i). */
    if (arr[1] != 20) return 4;
    if (*(arr + 3) != 40) return 5;

    /* EXCEPTION: as the operand of sizeof, the array does NOT decay; sizeof
     * yields the size of the whole array, not the size of a pointer. */
    if (sizeof arr != 4 * sizeof(int)) return 6;
    if (sizeof arr == sizeof(int *)) return 7;   /* would only match by luck */

    /* EXCEPTION: as the operand of unary &, no decay; &arr has type
     * "pointer to array of 4 int". Its numeric address equals &arr[0], but
     * advancing it by 1 advances by the whole array size. */
    int (*pa)[4] = &arr;
    if ((void *)pa != (void *)arr) return 8;
    /* (char*)(pa+1) - (char*)pa == sizeof(arr) */
    if ((char *)(pa + 1) - (char *)pa != (long)sizeof arr) return 9;

    /* EXCEPTION: a string literal initializing an array does not decay; the
     * array is initialized element-by-element and has the literal's size. */
    char s[] = "abc";
    if (sizeof s != 4) return 10;        /* 'a','b','c','\0' */
    if (s[0] != 'a' || s[3] != '\0') return 11;

    /* But a string literal in a value context decays to char*. */
    const char *q = "xy";
    if (q[0] != 'x' || q[1] != 'y' || q[2] != '\0') return 12;

    /* An array *member* decays exactly like a standalone array: "an expression
     * of type array of T" says nothing about how the object was reached. This
     * regressed once -- `g_s.buf` emitted a load of the member's first bytes
     * where its address belonged -- so each form is checked against &buf[0],
     * which reaches the same address without going through the decay path. */
    char *b = g_s.buf;                           /* decay in an initializer */
    if (b != &g_s.buf[0]) return 13;
    if (b[0] != 'a' || b[2] != 'c') return 14;
    if ((char *)g_s.buf != &g_s.buf[0]) return 15;  /* decay in a cast      */
    if (g_s.buf + 1 != &g_s.buf[1]) return 16;      /* decay in ptr arith   */

    /* sizeof and unary & are exceptions for a member array too. */
    if (sizeof g_s.buf != 8) return 17;
    if ((void *)&g_s.buf != (void *)g_s.buf) return 18;

    /* ...and through a pointer to the struct, where the base is an opaque
     * pointer value rather than a known object. */
    struct S *ps = &g_s;
    if (ps->buf != &g_s.buf[0]) return 19;
    if (ps->buf[1] != 'b') return 20;

    /* Nested one level down, so the member's own base is itself a member. */
    if (g_o.inner.buf != &g_o.inner.buf[0]) return 21;
    if (g_o.inner.buf[1] != 'y') return 22;

    /* An array of arrays: `m[1]` is an array element of array type. */
    if (g_m[1] != &g_m[1][0]) return 23;
    if (g_m[1][0] != 'r') return 24;

    return 0;
}
