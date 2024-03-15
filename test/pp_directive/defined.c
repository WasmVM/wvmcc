#define aaa

#if defined aaa
1;
#endif

#if defined(aaa)
2;
#endif

#if defined(bbb)
3;
#endif

#if defined bbb 
4;
#endif

#if 0
5;
#elif defined aaa 
6;
#endif
7;