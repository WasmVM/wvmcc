/* LANG-6.7.8-02 — a typedef name shares the ordinary-identifier name space
 * and can be shadowed by an inner-scope declaration of the same identifier
 * (C17 6.7.8p3, 6.2.3p1). */
typedef int T;

int main(void) {
    T outer = 5; /* T used as a type name */
    {
        int T = 3; /* shadows the typedef: T is now an object */
        if (T != 3) return 1;
        T = T + 1;
        if (T != 4) return 2;
    }
    T again = outer; /* typedef name is visible again */
    if (again != 5) return 3;
    return 0;
}
