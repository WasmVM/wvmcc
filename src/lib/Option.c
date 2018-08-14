#include <Option.h>

static void free_Option(Option* this){
    free(this->output_file);
    free(this);
}

Option* new_Option(int argc, const char* argv[]){
    Option* newOption = (Option*) malloc(sizeof(Option));
    newOption->free = free_Option;
    // Parse option
    for(int i = 0; i < argc; ++i){
        if(!strcmp(argv[i], "-o")){
            // TODO: set output
        }
    }
    return newOption;
}