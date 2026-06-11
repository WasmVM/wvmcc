/* LANG-6.6-04 — An integer constant expression shall only have operands
 * that are integer constants, enumeration constants, character constants,
 * sizeof/_Alignof results, and floating constants that are the IMMEDIATE
 * operands of casts (ISO C17 6.6p6). Here the floating constants are
 * operands of `+` (only the sum is the cast operand), so the expression is
 * not an ICE — yet an enumerator's value requires one (6.7.2.2p2). A
 * conforming compiler must reject this. */

enum e { E = (int)(1.5 + 1.5) };  /* ill-formed: floats are not immediate cast operands */
