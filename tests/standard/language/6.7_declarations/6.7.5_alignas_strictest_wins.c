/* LANG-6.7.5-01 — alignment specifier semantics (C17 6.7.5p6,p7):
 * "_Alignas(type)" has the same effect as "_Alignas(_Alignof(type))" (p6),
 * an alignment specifier may use a constant expression (p3), and when
 * multiple alignment specifiers appear in a declaration the effective
 * alignment is the strictest one specified (p7).
 *
 * Object alignment is observed through enclosing struct types: a structure
 * whose first member carries _Alignas must itself be at least that strictly
 * aligned, since the member at offset 0 must be suitably aligned.
 * Freestanding; file-scope _Static_assert only. */

/* _Alignas(type) ≡ _Alignas(_Alignof(type)) (6.7.5p6). */
struct by_type { _Alignas(double) char c; };
struct by_expr { _Alignas(_Alignof(double)) char c; };
_Static_assert(_Alignof(struct by_type) == _Alignof(struct by_expr),
               "_Alignas(double) must equal _Alignas(_Alignof(double))");
_Static_assert(_Alignof(struct by_type) >= _Alignof(double),
               "member _Alignas(double) must propagate to the struct");

/* _Alignas with an integer constant expression (6.7.5p3). */
struct by_const { _Alignas(2 * 8) char c; };
_Static_assert(_Alignof(struct by_const) >= 16,
               "_Alignas(16) via constant expression must align to 16");

/* Strictest alignment specifier wins (6.7.5p7). */
struct strictest { _Alignas(2) _Alignas(8) char c; };
_Static_assert(_Alignof(struct strictest) >= 8,
               "with _Alignas(2) and _Alignas(8) the effective alignment is 8");

/* _Alignas(0) has no effect (6.7.5p7) and may combine with another. */
struct with_zero { _Alignas(0) _Alignas(4) char c; };
_Static_assert(_Alignof(struct with_zero) >= 4,
               "_Alignas(0) is ignored; _Alignas(4) still applies");
