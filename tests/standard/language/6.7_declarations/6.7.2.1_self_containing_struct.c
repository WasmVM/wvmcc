/* LANG-6.7.2.1-02 — constraint violation (C17 6.7.2.1p3): a structure shall
 * not contain a member of incomplete type; in particular it shall not contain
 * an instance of itself (the type is incomplete until the closing brace).
 * A conforming compiler must reject this TU. */
struct node {
    int value;
    struct node next; /* error: member of incomplete type (self-containment) */
};
