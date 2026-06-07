/* LANG-6.3.2.1-03 — 6.3.2.1p3: array-to-pointer decay. Except when it is the
 * operand of sizeof, the unary & operator, or is a string literal used to
 * initialize an array, an expression of type "array of T" is converted to an
 * expression of type "pointer to T" that points to the first element of the
 * array object and is not an lvalue. */

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

    return 0;
}
