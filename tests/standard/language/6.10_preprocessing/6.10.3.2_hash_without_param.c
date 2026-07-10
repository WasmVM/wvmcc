/* LANG-6.10.3-07 — constraint violation (C17 6.10.3.2p1): in a function-like
 * macro, each # in the replacement list must be followed by a parameter.
 * A constraint on the definition itself — it must be diagnosed even though
 * M is never expanded. */
#define M(a) # b

int main(void) {
    return 0;
}
