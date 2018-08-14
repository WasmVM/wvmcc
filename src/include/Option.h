#ifndef WVMCC_OPTION_DEF
#define WVMCC_OPTION_DEF

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#ifndef WVMCC_VERSION
#define WVMCC_VERSION "dev"
#endif

typedef struct _Option{
    const char* output_file;
    const char** input_files;
    void (*free)(struct _Option* this);
} Option;

Option* new_Option(int argc, const char* argv[]);

#endif