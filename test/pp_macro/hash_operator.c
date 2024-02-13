#define foo(VAL) char* test = # VAL;
foo(str)
foo(str "s")
foo(str "\\s")
#define foo1(__VA_ARGS__) char* test = # __VA_ARGS__;
foo(str,s)