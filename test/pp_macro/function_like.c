#define foo1() int a;
foo1()
#define foo2(NAME) int NAME;
foo2(NAME)
#define foo3(TYPE, NAME) TYPE NAME;
foo3(float, cc)
#define foo4(NAME) struct {foo2(NAME)};
foo4(dd)