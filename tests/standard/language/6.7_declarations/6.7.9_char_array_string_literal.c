/* LANG-6.7.9-09 — an array of character type may be initialized by a string
 * literal: successive characters initialize the elements, including the
 * terminating null if there is room or the array is of unknown size; the
 * terminator is dropped if the array exactly fits the characters
 * (C17 6.7.9p14). Wide-character arrays take wide string literals
 * (C17 6.7.9p15). */
#include <stddef.h>

char a[] = "abc";  /* unknown size: 4 elements including '\0' */
char b[3] = "abc"; /* exactly fits the 3 characters; terminator dropped */
char c[8] = "hi";  /* remaining elements zero-initialized (static duration) */
wchar_t w[] = L"xy";

int main(void) {
    if (sizeof a != 4) return 1;
    if (a[0] != 'a' || a[1] != 'b' || a[2] != 'c' || a[3] != '\0') return 2;

    if (sizeof b != 3) return 3;
    if (b[0] != 'a' || b[1] != 'b' || b[2] != 'c') return 4;

    if (c[0] != 'h' || c[1] != 'i') return 5;
    for (int i = 2; i < 8; ++i) {
        if (c[i] != '\0') return 6;
    }

    if (sizeof w / sizeof w[0] != 3) return 7;
    if (w[0] != L'x' || w[1] != L'y' || w[2] != L'\0') return 8;

    return 0;
}
