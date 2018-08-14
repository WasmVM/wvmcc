#ifndef WVMCC_OPTION_DEF
#define WVMCC_OPTION_DEF

#include <string.h>
#include <stdlib.h>

typedef struct {
    char* output_file;

} Option;

Option* new_Option(int argc, char* argv[]);

#endif