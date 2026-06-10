/* LANG-6.7.8-01 — a typedef declaration introduces a synonym for the type,
 * not a new type (C17 6.7.8p3). Objects declared via the typedef name are
 * interchangeable with the underlying type without conversions. */
typedef int Length;
typedef int *IntPtr;
typedef int Pair[2];

int main(void) {
    Length n = 42;
    int *p = &n; /* Length is exactly int, so int* points at it directly */
    *p = 7;
    if (n != 7) return 1;

    IntPtr q = p; /* IntPtr is exactly int* */
    if (*q != 7) return 2;

    Pair a = {1, 2}; /* Pair is exactly int[2] */
    int *e = a;
    if (e[0] + e[1] != 3) return 3;

    typedef Length Meters; /* a typedef of a typedef is still the same type */
    Meters m = n;
    if (m != 7) return 4;

    return 0;
}
