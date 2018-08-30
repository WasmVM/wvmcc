#include <skypat/skypat.h>

#define restrict __restrict__
extern "C"{
    #include <ByteBuffer.h>
    #include <Lexer.h>
}
#undef restrict

SKYPAT_F(Lexer_unittest, input_not_equal_output)
{
    Buffer* input[3] = {
        new Buffer,
        new Buffer,
        new Buffer
    };
    Buffer* output[2] = {
        new Buffer,
        new Buffer
    };
    Lexer* lexer = new_Lexer(input, 3, output, 2);
    EXPECT_EQ(lexer, NULL);
    for(int i = 0; i < 3; ++i){
        delete input[i];
    }
    for(int i = 0; i < 2; ++i){
        delete output[i];
    }
}

SKYPAT_F(Lexer_unittest, create_delete)
{
    Buffer* input[2] = {
        new Buffer,
        new Buffer
    };
    Buffer* output[2] = {
        new Buffer,
        new Buffer
    };
    Lexer* lexer = new_Lexer(input, 2, output, 2);
    EXPECT_NE(lexer, NULL);
    EXPECT_EQ(lexer->input_count, 2);
    EXPECT_EQ(lexer->output_count, 2);
    for(int i = 0; i < 2; ++i){
        delete input[i];
        delete output[i];
    }
}

SKYPAT_F(Lexer_unittest, unsigned)
{
    // Input
    ByteBuffer* input = new_ByteBuffer(8);
    set_ByteBuffer(input, 0, "unsigned", 8);
    // Outputs
    Buffer* output = (Buffer*)new_TokenBuffer();
    Lexer* lexer = new_Lexer(&input, 1, &output, 1);
    lexer->run(lexer);
    // Check
    Token* token = (Token*)listAt((List*)output->data, 0);
    EXPECT_EQ(token->type, Token_Keyword);
    EXPECT_EQ(token->value.keyword, Keyword_unsigned);
    // Clean
    input->free(&input);
    output->free(&output);
    lexer->free(&lexer);
}

SKYPAT_F(Lexer_unittest, normal)
{
    // Input
    ByteBuffer* input = new_ByteBuffer(5);
    set_ByteBuffer(input, 0, "teSt0", 5);
    // Output
    Buffer* output = (Buffer*)new_TokenBuffer();
    Lexer* lexer = new_Lexer(&input, 1, &output, 1);
    lexer->run(lexer);
    // Check
    Token* token = (Token*)listAt((List*)output->data, 0);
    EXPECT_EQ(token->type, Token_Identifier);
    EXPECT_FALSE(strcmp(token->value.identifier, "teSt0"));
    // Clean
    input->free(&input);
    output->free(&output);
    lexer->free(&lexer);
}

SKYPAT_F(Lexer_unittest, number_prefixed)
{
    // Input
    ByteBuffer* input = new_ByteBuffer(5);
    set_ByteBuffer(input, 0, "0test", 5);
    // Output
    Buffer* output = (Buffer*)new_TokenBuffer();
    Lexer* lexer = new_Lexer(&input, 1, &output, 1);
    lexer->run(lexer);
    // Check
    EXPECT_EQ(((List*)output->data)->size, 0);
    // Clean
    input->free(&input);
    output->free(&output);
    lexer->free(&lexer);
}

SKYPAT_F(Lexer_unittest, Integer)
{

}

SKYPAT_F(Lexer_unittest, Floating)
{

}

SKYPAT_F(Lexer_unittest, Character)
{

}

SKYPAT_F(Lexer_unittest, String)
{

}

SKYPAT_F(Lexer_unittest, Punctuator)
{

}