/* LANG-6.7.9-03 — constraint: the type of the entity to be initialized shall
 * be an array of unknown size or a complete object type that is not a VLA
 * (C17 6.7.9p3). Initializing an incomplete struct type must be rejected. */
struct S; /* incomplete type */

struct S s = {1}; /* constraint violation: initializer for incomplete type */

int main(void) {
    return 0;
}
