// Phase 2 e2e — string literals lower to data segments and char* assignment.
//
// Just exercising literal emission and a char* parameter pass. We don't read
// bytes back because the current wvmcc codegen does not emit i32.load8 for
// `char *p; p[i]` and does not emit valid pointer comparisons (separate bugs).
void take_str(char *s) { (void)s; }

int main(void) {
    char *s = "hello";
    char *t = "world";
    take_str(s);
    take_str(t);
    return 0;
}
