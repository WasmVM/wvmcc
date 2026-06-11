/* LANG-5.1.2.2.1-03 — 5.1.2.2.1p2: at program startup, argc is nonnegative,
 * argv[argc] is a null pointer, and the strings argv[0..argc-1] are
 * modifiable by the program. */
int main(int argc, char *argv[])
{
    if (argc < 0) return 1;                 /* argc nonnegative */
    if (argv[argc] != 0) return 2;          /* argv[argc] is a null pointer */
    if (argc > 0) {
        char saved = argv[0][0];
        argv[0][0] = 'X';                   /* argv strings are modifiable */
        if (argv[0][0] != 'X') return 3;
        argv[0][0] = saved;
    }
    return 0;
}
