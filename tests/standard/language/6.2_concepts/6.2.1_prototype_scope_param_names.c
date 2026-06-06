/* LANG-6.2.1-04 — 6.2.1p4: a parameter name in a function declarator has
 * function-prototype scope; it does not leak past the declarator, so the
 * same name may be reused freely elsewhere. */
int g(int count);      /* 'count' has prototype scope only */

int g(int n) { return n; }

int main(void)
{
    int count = 7;     /* unrelated object; no clash with the prototype name */
    if (g(count) != 7) return 1;
    return 0;
}
