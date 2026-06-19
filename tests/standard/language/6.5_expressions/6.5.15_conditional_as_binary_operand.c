/* LANG-6.5.15-05 — A conditional expression `c ? a : b` used as an operand of a
 * binary operator yields the value of the chosen branch, and the *other*
 * operand of the enclosing operator is preserved regardless of which branch is
 * taken (ISO C17 6.5.15p4). This is a regression guard: an earlier codegen bug
 * emitted the ternary as a typed-result `if` block sitting above the already-
 * pushed left operand on the WasmVM value stack; when the condition was true the
 * interpreter dropped that left operand, so `X + (c ? a : b)` computed `a + a`
 * (or otherwise lost `X`). The condition-false path was unaffected. */

static int id(int x) { return x; }

int main(void)
{
    int b = 42;

    /* Condition true: left operand `b` must survive. */
    if (b + (b == 42 ? 0 : 1000) != 42) return 1;
    if (b + (b == 42 ? 7 : 1000) != 49) return 2;

    /* Left operand as a literal, condition true. */
    if (100 + (b == 42 ? 0 : 1000) != 100) return 3;

    /* Condition false: previously correct, must stay correct. */
    if (b + (b == 7 ? 0 : 1) != 43) return 4;

    /* Conditional on the left of the binary operator, condition true. */
    if ((b == 42 ? 7 : 1000) + b != 49) return 5;

    /* Nested / function-call operand so the value isn't a foldable constant. */
    if (id(b) + (b == 42 ? 8 : 1000) != 50) return 6;

    /* Wider (i64) branches under an enclosing add. */
    {
        long x = 42;
        if ((int)(x + (x == 42 ? 0L : 1000L)) != 42) return 7;
    }

    return 0;
}
