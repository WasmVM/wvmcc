// Phase 2 verification: string literals in data segment
// Expected: data segment at offset 8 with correct bytes
void use_str(char *s);

void test_string() {
    char *s = "hello";
    use_str(s);
}

void test_two_strings() {
    char *a = "foo";
    char *b = "bar";
    use_str(a);
    use_str(b);
}
