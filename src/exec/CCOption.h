#ifndef WVMCC_CCOPTION_DEF
#define WVMCC_CCOPTION_DEF

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#ifndef WVMCC_VERSION
#define WVMCC_VERSION "dev"
#endif

typedef struct _CCOption{
    const char* output_file;
    const char** input_files;
    size_t input_count;
} CCOption;

CCOption* new_CCOption(int argc, const char* argv[]);
void free_CCOption(CCOption** option);

#endif