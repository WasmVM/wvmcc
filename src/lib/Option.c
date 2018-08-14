#include <Option.h>

static void free_Option(Option* this){
    free(this);
}

static void print_usage(Option* this){
    fprintf(stderr, "Usage: wvmcc [options] file...\n");
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -o <file>      Place the output into <file>.\n");
    fprintf(stderr, "  --version      Display compiler version information\n");
    fprintf(stderr, "  --help, -h     Display usage information\n");
    free(this);
    exit(-1);
}

static void print_version(Option* this){
    printf("wvmcc %s\n", WVMCC_VERSION);
    free(this);
    exit(0);
}

Option* new_Option(int argc, const char* argv[]){
    Option* newOption = (Option*) malloc(sizeof(Option));
    newOption->free = free_Option;
    // Parse option
    for(int i = 0; i < argc; ++i){
        if(!strcmp(argv[i], "-o")){
            if(i < argc - 1){
                newOption->output_file = argv[i+1];
                i += 1;
            }else{
                print_usage(newOption);
            }
        }else if(!strcmp(argv[i], "--help") || !strcmp(argv[i], "-h")){
            print_usage(newOption);
        }else if(!strcmp(argv[i], "--version")){
            print_version(newOption);
        }
    }
    return newOption;
}