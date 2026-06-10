/* LANG-6.7.2-02 — 6.7.2p2 (constraint): at least one type specifier shall be
 * given in the declaration specifiers, and the multiset of type specifiers
 * shall be one of the listed valid multisets. `unsigned signed int` is not a
 * valid multiset, so a conforming compiler must reject this declaration. */

unsigned signed int bad_multiset; /* constraint violation: invalid multiset */

int main(void) {
    return 0;
}
