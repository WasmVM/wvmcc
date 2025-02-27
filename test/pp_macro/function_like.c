#define foo1()int a;
foo1()
#define foo2(NAME)int NAME
foo2(NAME);
#define foo3(TYPE, NAME)TYPE NAME;
foo3(float,cc)
#define foo4(NAME)struct {foo2(NAME)};
foo4(dd)
#define foo5(TYPE, NAME, VAL)TYPE NAME = VAL;
foo5(char,ee,3)
#define foo6(DEF, VAL)DEF = VAL;
foo6(foo2(ff),4)
#define foo7(...)foo3(__VA_ARGS__)
foo7(unsigned,gg)
#define foo8(SP, ...)SP foo5(__VA_ARGS__)
foo8(const,char,hh,5)
#define f(a)a*g
#define g(a)f(a)
f(2)(9)
