/* LANG-6.7.2.3-03 — a type specifier of the form `enum identifier` without an
   enumerator list shall appear only after the type it specifies is complete
   (6.7.2.3p3, constraint). Here `enum E` is used before its definition. */
enum E e;
enum E { A, B };
