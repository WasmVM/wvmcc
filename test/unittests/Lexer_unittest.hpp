#include <skypat/skypat.h>

#define restrict __restrict__
#define _Bool bool
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

SKYPAT_F(Lexer_unittest, Keyword_normal)
{
    // Input
    ByteBuffer* input[44];
    input[0] = new_ByteBuffer(5);
    set_ByteBuffer(input[0], 0, "auto", 5);
    input[1] = new_ByteBuffer(6);
    set_ByteBuffer(input[1], 0, "break", 6);
    input[2] = new_ByteBuffer(5);
    set_ByteBuffer(input[2], 0, "case", 5);
    input[3] = new_ByteBuffer(5);
    set_ByteBuffer(input[3], 0, "char", 5);
    input[4] = new_ByteBuffer(6);
    set_ByteBuffer(input[4], 0, "const", 6);
    input[5] = new_ByteBuffer(9);
    set_ByteBuffer(input[5], 0, "continue", 9);
    input[6] = new_ByteBuffer(8);
    set_ByteBuffer(input[6], 0, "default", 8);
    input[7] = new_ByteBuffer(3);
    set_ByteBuffer(input[7], 0, "do", 3);
    input[8] = new_ByteBuffer(7);
    set_ByteBuffer(input[8], 0, "double", 7);
    input[9] = new_ByteBuffer(5);
    set_ByteBuffer(input[9], 0, "else", 5);
    input[10] = new_ByteBuffer(5);
    set_ByteBuffer(input[10], 0, "enum", 5);
    input[11] = new_ByteBuffer(7);
    set_ByteBuffer(input[11], 0, "extern", 7);
    input[12] = new_ByteBuffer(6);
    set_ByteBuffer(input[12], 0, "float", 6);
    input[13] = new_ByteBuffer(4);
    set_ByteBuffer(input[13], 0, "for", 4);
    input[14] = new_ByteBuffer(5);
    set_ByteBuffer(input[14], 0, "goto", 5);
    input[15] = new_ByteBuffer(3);
    set_ByteBuffer(input[15], 0, "if", 3);
    input[16] = new_ByteBuffer(7);
    set_ByteBuffer(input[16], 0, "inline", 7);
    input[17] = new_ByteBuffer(4);
    set_ByteBuffer(input[17], 0, "int", 4);
    input[18] = new_ByteBuffer(5);
    set_ByteBuffer(input[18], 0, "long", 5);
    input[19] = new_ByteBuffer(9);
    set_ByteBuffer(input[19], 0, "register", 9);
    input[20] = new_ByteBuffer(9);
    set_ByteBuffer(input[20], 0, "restrict", 9);
    input[21] = new_ByteBuffer(7);
    set_ByteBuffer(input[21], 0, "return", 7);
    input[22] = new_ByteBuffer(6);
    set_ByteBuffer(input[22], 0, "short", 6);
    input[23] = new_ByteBuffer(7);
    set_ByteBuffer(input[23], 0, "signed", 7);
    input[24] = new_ByteBuffer(7);
    set_ByteBuffer(input[24], 0, "sizeof", 7);
    input[25] = new_ByteBuffer(7);
    set_ByteBuffer(input[25], 0, "static", 6);
    input[26] = new_ByteBuffer(7);
    set_ByteBuffer(input[26], 0, "struct", 7);
    input[27] = new_ByteBuffer(7);
    set_ByteBuffer(input[27], 0, "switch", 7);
    input[28] = new_ByteBuffer(8);
    set_ByteBuffer(input[28], 0, "typedef", 8);
    input[29] = new_ByteBuffer(6);
    set_ByteBuffer(input[29], 0, "union", 6);
    input[30] = new_ByteBuffer(9);
    set_ByteBuffer(input[30], 0, "unsigned", 9);
    input[31] = new_ByteBuffer(5);
    set_ByteBuffer(input[31], 0, "void", 5);
    input[32] = new_ByteBuffer(9);
    set_ByteBuffer(input[32], 0, "volatile", 9);
    input[33] = new_ByteBuffer(6);
    set_ByteBuffer(input[33], 0, "while", 6);
    input[34] = new_ByteBuffer(9);
    set_ByteBuffer(input[34], 0, "_Alignas", 9);
    input[35] = new_ByteBuffer(9);
    set_ByteBuffer(input[35], 0, "_Alignof", 9);
    input[36] = new_ByteBuffer(8);
    set_ByteBuffer(input[36], 0, "_Atomic", 8);
    input[37] = new_ByteBuffer(6);
    set_ByteBuffer(input[37], 0, "_Bool", 6);
    input[38] = new_ByteBuffer(9);
    set_ByteBuffer(input[38], 0, "_Complex", 9);
    input[39] = new_ByteBuffer(9);
    set_ByteBuffer(input[39], 0, "_Generic", 9);
    input[40] = new_ByteBuffer(11);
    set_ByteBuffer(input[40], 0, "_Imaginary", 11);
    input[41] = new_ByteBuffer(10);
    set_ByteBuffer(input[41], 0, "_Noreturn", 10);
    input[42] = new_ByteBuffer(15);
    set_ByteBuffer(input[42], 0, "_Static_assert", 15);
    input[43] = new_ByteBuffer(14);
    set_ByteBuffer(input[43], 0, "_Thread_local", 14);

    // Outputs
    Buffer* output[44];
    for(size_t i = 0; i < 44; ++i){
        output[i] = (Buffer*)new_TokenBuffer();
    }
    Lexer* lexer = new_Lexer(input, 44, output, 44);
    EXPECT_EQ(lexer->run(lexer), 0);
    // Check
    for(size_t i = 0; i < 44; ++i){
        Token* token = (Token*)listAt((List*)output[i]->data, 0);
        EXPECT_EQ(token->type, Token_Keyword);
        EXPECT_EQ(token->value.keyword, i+1);
    }
    // Clean
    for(size_t i = 0; i < 44; ++i){
        input[i]->free(&input[i]);
        output[i]->free(&output[i]);
    }
    lexer->free(&lexer);
}

SKYPAT_F(Lexer_unittest, Identifier_normal)
{
    // Input
    ByteBuffer* input = new_ByteBuffer(5);
    set_ByteBuffer(input, 0, "teSt0", 5);
    // Output
    Buffer* output = (Buffer*)new_TokenBuffer();
    Lexer* lexer = new_Lexer(&input, 1, &output, 1);
    ASSERT_EQ(lexer->run(lexer), 0);
    // Check
    Token* token = (Token*)listAt((List*)output->data, 0);
    EXPECT_EQ(token->type, Token_Identifier);
    EXPECT_FALSE(strcmp(token->value.identifier, "teSt0"));
    // Clean
    input->free(&input);
    output->free(&output);
    lexer->free(&lexer);
}

SKYPAT_F(Lexer_unittest, Integer_decimal)
{
    // Input
    ByteBuffer* input[9];
    input[0] = new_ByteBuffer(5);
    set_ByteBuffer(input[0], 0, "1234", 5);
    input[1] = new_ByteBuffer(6);
    set_ByteBuffer(input[1], 0, "1234u", 6);
    input[2] = new_ByteBuffer(6);
    set_ByteBuffer(input[2], 0, "1234U", 6);
    input[3] = new_ByteBuffer(7);
    set_ByteBuffer(input[3], 0, "1234l", 7);
    input[4] = new_ByteBuffer(7);
    set_ByteBuffer(input[4], 0, "1234lu", 7);
    input[5] = new_ByteBuffer(8);
    set_ByteBuffer(input[5], 0, "1234ull", 8);
    input[6] = new_ByteBuffer(8);
    set_ByteBuffer(input[6], 0, "1234LL", 8);
    input[7] = new_ByteBuffer(7);
    set_ByteBuffer(input[7], 0, "1234Ul", 7);
    input[8] = new_ByteBuffer(7);
    set_ByteBuffer(input[8], 0, "1234Lu", 7);
    // Output
    Buffer* output[9];
    for(size_t i = 0; i < 9; ++i){
        output[i] = (Buffer*) new_TokenBuffer();
    }
    // Run
    Lexer* lexer = new_Lexer(input, 9, output, 9);
    ASSERT_EQ(lexer->run(lexer), 0);
    // Check
    Token* token = (Token*)listAt((List*)output[0]->data, 0);
    EXPECT_EQ(token->type, Token_Integer);
    EXPECT_EQ(token->value.integer, 1234);
    EXPECT_EQ(token->byteSize, 4);
    EXPECT_FALSE(token->isUnsigned);
    token = (Token*)listAt((List*)output[1]->data, 0);
    EXPECT_EQ(token->type, Token_Integer);
    EXPECT_EQ(token->value.integer, 1234);
    EXPECT_EQ(token->byteSize, 4);
    EXPECT_TRUE(token->isUnsigned);
    token = (Token*)listAt((List*)output[2]->data, 0);
    EXPECT_EQ(token->type, Token_Integer);
    EXPECT_EQ(token->value.integer, 1234);
    EXPECT_EQ(token->byteSize, 4);
    EXPECT_TRUE(token->isUnsigned);
    token = (Token*)listAt((List*)output[3]->data, 0);
    EXPECT_EQ(token->type, Token_Integer);
    EXPECT_EQ(token->value.integer, 1234);
    EXPECT_EQ(token->byteSize, 4);
    EXPECT_FALSE(token->isUnsigned);
    token = (Token*)listAt((List*)output[4]->data, 0);
    EXPECT_EQ(token->type, Token_Integer);
    EXPECT_EQ(token->value.integer, 1234);
    EXPECT_EQ(token->byteSize, 4);
    EXPECT_TRUE(token->isUnsigned);
    token = (Token*)listAt((List*)output[5]->data, 0);
    EXPECT_EQ(token->type, Token_Integer);
    EXPECT_EQ(token->value.integer, 1234);
    EXPECT_EQ(token->byteSize, 8);
    EXPECT_TRUE(token->isUnsigned);
    token = (Token*)listAt((List*)output[6]->data, 0);
    EXPECT_EQ(token->type, Token_Integer);
    EXPECT_EQ(token->value.integer, 1234);
    EXPECT_EQ(token->byteSize, 8);
    EXPECT_FALSE(token->isUnsigned);
    token = (Token*)listAt((List*)output[7]->data, 0);
    EXPECT_EQ(token->type, Token_Integer);
    EXPECT_EQ(token->value.integer, 1234);
    EXPECT_EQ(token->byteSize, 4);
    EXPECT_TRUE(token->isUnsigned);
    token = (Token*)listAt((List*)output[8]->data, 0);
    EXPECT_EQ(token->type, Token_Integer);
    EXPECT_EQ(token->value.integer, 1234);
    EXPECT_EQ(token->byteSize, 4);
    EXPECT_TRUE(token->isUnsigned);
    // Clean
    for(size_t i = 0; i < 9; ++i){
        input[i]->free(&input[i]);
        output[i]->free(&output[i]);
    }
    lexer->free(&lexer);
}
SKYPAT_F(Lexer_unittest, Integer_octal)
{
    // Input
    ByteBuffer* input = new_ByteBuffer(4);
    set_ByteBuffer(input, 0, "023", 4);
    // Output
    Buffer* output = (Buffer*)new_TokenBuffer();
    Lexer* lexer = new_Lexer(&input, 1, &output, 1);
    ASSERT_EQ(lexer->run(lexer), 0);
    // Check
    Token* token = (Token*)listAt((List*)output->data, 0);
    EXPECT_EQ(token->type, Token_Integer);
    EXPECT_EQ(token->value.integer, 19);
    // Clean
    input->free(&input);
    output->free(&output);
    lexer->free(&lexer);
}
SKYPAT_F(Lexer_unittest, Integer_hexadecimal)
{
    // Input
    ByteBuffer* input = new_ByteBuffer(4);
    set_ByteBuffer(input, 0, "0xa", 4);
    // Output
    Buffer* output = (Buffer*)new_TokenBuffer();
    Lexer* lexer = new_Lexer(&input, 1, &output, 1);
    ASSERT_EQ(lexer->run(lexer), 0);
    // Check
    Token* token = (Token*)listAt((List*)output->data, 0);
    EXPECT_EQ(token->type, Token_Integer);
    EXPECT_EQ(token->value.integer, 10);
    // Clean
    input->free(&input);
    output->free(&output);
    lexer->free(&lexer);
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