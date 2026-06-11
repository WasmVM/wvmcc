/* LANG-6.3.2.2-01 — 6.3.2.2p1: the (nonexistent) value of a void expression
 * shall not be used in any way, and implicit or explicit conversions (except
 * to void) shall not be applied to such an expression. Using a void-typed
 * expression where a value is required is a constraint violation. */
void g(void) { }

int main(void)
{
    int x = g();   /* ill-formed: void expression's value cannot be used/converted */
    return x;
}
