#define foo1() int a;
foo1()
#define foo2(NAME) int NAME
foo2(NAME);
#define foo3(TYPE, NAME) TYPE NAME;
foo3(float, cc)
#define foo4(NAME) struct {foo2(NAME)};
foo4(dd)
#define foo5(TYPE, NAME, VAL) TYPE NAME = VAL;
foo5(char, ee, 3)
#define foo6(DEF, VAL) DEF = VAL;
foo6(foo2(ff), 4)