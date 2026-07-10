/* LANG-6.10.3-09 — constraint violation (C17 6.10.3p2): an identifier
 * currently defined as a macro may be redefined only by an identical
 * replacement list. X's second definition differs and must be diagnosed. */
#define X 1
#define X 2

int main(void) {
    return X;
}
