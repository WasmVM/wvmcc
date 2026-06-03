// #79 e2e — function pointers stored in file-scope variables and an array,
// then called through call_indirect. phase4-func-pointer covers function
// pointers in locals/params; this covers file-scope scalar + array storage in
// linear memory (the atexit() prerequisite) plus void-returning indirect calls.
int acc;
void a(void) { acc += 1; }
void b(void) { acc += 10; }
void c(void) { acc += 100; }

void (*g)(void);        // file-scope scalar function pointer
void (*tbl[4])(void);   // file-scope array of function pointers
int n;

int main(void) {
    g = a;
    g();                          // acc = 1

    tbl[n++] = b;
    tbl[n++] = c;
    while (n > 0) tbl[--n]();      // LIFO: c (+100) then b (+10) -> acc = 111

    return acc == 111 ? 0 : 1;
}
