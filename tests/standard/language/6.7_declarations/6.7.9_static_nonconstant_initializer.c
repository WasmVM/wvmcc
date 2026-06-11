/* LANG-6.7.9-04 — constraint: all the expressions in an initializer for an
 * object that has static or thread storage duration shall be constant
 * expressions or string literals (C17 6.7.9p4). */
int f(void) {
    return 3;
}

int g = f(); /* constraint violation: non-constant initializer for static-duration object */

int main(void) {
    return 0;
}
