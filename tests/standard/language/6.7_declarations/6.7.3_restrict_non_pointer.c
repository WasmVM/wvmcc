/* LANG-6.7.3-02 — restrict on a non-pointer type (C17 6.7.3p2):
 * Constraint: types other than pointer types whose referenced type is an
 * object type shall not be restrict-qualified. A conforming compiler must
 * reject restrict applied to an ordinary object type. */
restrict int x; /* constraint violation: restrict-qualified int */

int main(void) {
    return 0;
}
