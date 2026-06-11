/* LANG-6.7.1-01 — storage-class specifiers recognized (C17 6.7.1p1,p5):
 * typedef, extern, static, _Thread_local, auto, and register are all valid
 * storage-class specifiers; objects declared with each behave as ordinary
 * objects of their type. */
typedef int my_int;

extern int ext_obj;            /* extern declaration ...   */
int ext_obj = 7;               /* ... and its definition   */

static int file_static = 3;

_Thread_local int tls_obj = 11;

int main(void) {
    my_int t = 1;
    auto int a = 2;
    register int r = 5;
    static int local_static = 4;

    if (t != 1) return 1;
    if (a != 2) return 2;
    if (r != 5) return 3;
    if (local_static != 4) return 4;
    if (file_static != 3) return 5;
    if (ext_obj != 7) return 6;
    if (tls_obj != 11) return 7;
    tls_obj = 13;
    if (tls_obj != 13) return 8;
    return 0;
}
