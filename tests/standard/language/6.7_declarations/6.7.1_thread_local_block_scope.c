/* LANG-6.7.1-03 — _Thread_local at block scope requires static or extern
 * (C17 6.7.1p3): "In the declaration of an identifier with block scope, if
 * the declaration specifiers include _Thread_local, they shall also include
 * either static or extern."  A bare block-scope `_Thread_local int` is a
 * constraint violation a conforming compiler MUST reject. */
int main(void) {
    _Thread_local int x = 0; /* missing static/extern: constraint violation */
    return x;
}
