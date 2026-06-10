/* LIBC-stdio-macros-01 — C17 7.21.1p3: <stdio.h> defines the macros
 * NULL, _IOFBF/_IOLBF/_IONBF (distinct ICEs), BUFSIZ (ICE >= 256),
 * EOF (negative int ICE), FOPEN_MAX (ICE >= 8), FILENAME_MAX,
 * L_tmpnam, SEEK_CUR/SEEK_END/SEEK_SET (distinct ICEs), TMP_MAX
 * (ICE >= 25). Verify=static-assert (compiled -ffreestanding). */
#include <stdio.h>

#ifndef NULL
#error "NULL must be defined by <stdio.h>"
#endif

/* EOF: integer constant expression with type int and a negative value. */
_Static_assert(EOF < 0, "EOF must be negative");

/* BUFSIZ: at least 256. */
_Static_assert(BUFSIZ >= 256, "BUFSIZ must be at least 256");

/* FOPEN_MAX: at least eight files guaranteed openable. */
_Static_assert(FOPEN_MAX >= 8, "FOPEN_MAX must be at least 8");

/* TMP_MAX: at least 25 unique tmpnam names. */
_Static_assert(TMP_MAX >= 25, "TMP_MAX must be at least 25");

/* FILENAME_MAX / L_tmpnam: sizes for filename arrays — must be positive. */
_Static_assert(FILENAME_MAX > 0, "FILENAME_MAX must be a positive ICE");
_Static_assert(L_tmpnam > 0, "L_tmpnam must be a positive ICE");

/* Buffering-mode macros: distinct integer constant expressions. */
_Static_assert(_IOFBF != _IOLBF, "_IOFBF and _IOLBF must be distinct");
_Static_assert(_IOLBF != _IONBF, "_IOLBF and _IONBF must be distinct");
_Static_assert(_IOFBF != _IONBF, "_IOFBF and _IONBF must be distinct");

/* Seek-origin macros: distinct integer constant expressions. */
_Static_assert(SEEK_SET != SEEK_CUR, "SEEK_SET and SEEK_CUR must differ");
_Static_assert(SEEK_CUR != SEEK_END, "SEEK_CUR and SEEK_END must differ");
_Static_assert(SEEK_SET != SEEK_END, "SEEK_SET and SEEK_END must differ");
