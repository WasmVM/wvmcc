/* LANG-6.4.8-02 — constraint violation (C17 6.4.8p4): a pp-number that does
 * not convert to a valid integer or floating constant in translation phase 7
 * must be diagnosed. `1abc` is a single pp-number but no valid constant;
 * stoull's prefix parse used to fold it to 1 silently. */
int b = 1abc;

int main(void) {
    return 0;
}
