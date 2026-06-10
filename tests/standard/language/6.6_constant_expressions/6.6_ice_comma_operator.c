/* LANG-6.6-02 — Constraint: constant expressions shall not contain
 * assignment, increment, decrement, function-call, or comma operators,
 * except when they are contained within a subexpression that is not
 * evaluated (ISO C17 6.6p3). An evaluated comma operator inside an enum
 * value (an integer constant expression) violates this constraint and a
 * conforming compiler must reject it. */

enum e { E = (1, 2) };  /* ill-formed: evaluated comma operator in an ICE */
