#include <Lexer.h>

static void trim(char** dataPtr){
    while(**dataPtr == ' ' || **dataPtr == '\t' || **dataPtr == '\r' || **dataPtr == '\n'){
        ++(*dataPtr);
    }
}

static int run_lexer(Lexer* lexer){
    for(size_t i = 0; i < lexer->input_count; ++i){
        ByteBuffer* input = lexer->input[i];
        TokenBuffer* output = (TokenBuffer*)lexer->output[i];
        char *cursor = (char*)input->data;
        char *end = cursor + input->length;
        trim(&cursor);
        while(cursor < end){
            Token* token = NULL;
            if( (token = lex_String(&cursor)) ||
                (token = lex_Character(&cursor)) ||
                (token = lex_Keyword(&cursor)) ||
                (token = lex_Floating(&cursor)) ||
                (token = lex_Integer(&cursor)) ||
                (token = lex_Identifier(&cursor)) ||
                (token = lex_Punctuator(&cursor))
            ){
                output->add(output, token);
                ++cursor;
            }else{
                // TODO: Error handling
                fprintf(stderr, "[Lexer] Unknown token.\n");
                return -1;
            }
        }
    }
    return 0;
}

static void free_lexer(Lexer** lexer){
    free(*lexer);
    *lexer = NULL;
}

Pass* new_Lexer(Buffer** input, size_t input_count, Buffer** output, size_t output_count){
    // Input count should equal output count
    if(input_count != output_count){
        fprintf(stderr, "error: input count not equal to output in Lexer\n");
        return NULL;
    }
    // Create pass
    Pass *pass = (Pass*) malloc(sizeof(Pass));
    pass->input = input;
    pass->input_count = input_count;
    pass->output = output;
    pass->output_count = output_count;
    pass->contextMap = NULL;
    pass->run = run_lexer;
    pass->free = free_lexer;
    return pass;
}