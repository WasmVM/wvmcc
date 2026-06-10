/* LANG-6.7.4-04 — inline definition restrictions (C17 6.7.4p3):
 * "An inline definition of a function with external linkage shall not
 * contain a definition of a modifiable object with static or thread
 * storage duration, and shall not contain a reference to an identifier
 * with internal linkage."  `counter` below is an inline definition with
 * external linkage (declared inline, never with extern) that defines a
 * modifiable static-duration object — a constraint violation a conforming
 * compiler MUST reject. */
inline int counter(void) {
    static int n = 0;   /* modifiable static-duration object: ill-formed */
    return n++;
}

int main(void) { return 0; }
