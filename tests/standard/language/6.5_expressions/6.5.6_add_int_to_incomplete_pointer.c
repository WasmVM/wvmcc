/* LANG-6.5.6-11 — 6.5.6p2 (ISO C17): for pointer + integer, the pointer operand
 * must be a pointer to a *complete* object type. Performing arithmetic on a
 * pointer to an incomplete type (here, an incomplete struct) violates this
 * constraint and must be rejected. Verify=compile-fail. */
struct incomplete;                 /* incomplete object type */
int f(struct incomplete *p) {
    p = p + 1;                     /* arithmetic on pointer to incomplete type */
    return p != 0;
}
