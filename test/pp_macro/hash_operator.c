#define foo(VAL) char* test = # VAL;
foo(str)
foo(str "s")
foo(str "\\s")
#define foo1(...) char* test = # __VA_ARGS__;
foo1(str,s)