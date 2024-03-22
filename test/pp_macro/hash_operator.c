#define foo(VAL) char* test = # VAL;
foo(str)
foo(str "s")
foo(str "\\s")
#define foo1(...) char* test = # __VA_ARGS__;
foo1(str,s)
#define foo2(Val1, Val2) Val1 ## Val2
foo2(i, nt) a;
#define foo3(Val1, Val2, Val3) Val1 ## Val2 ## Val3
foo3(i, n, t) b;
#define foo4(Val1, Val2, Val3) Val1 ## Val2 Val3
foo4(i, nt, c) ;
#define hash_hash # ## #
#define mkstr(s) # s
#define join(i, j) mkstr(i hash_hash j)
char d[] = join(x, y);