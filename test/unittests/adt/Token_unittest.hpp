#include <skypat/skypat.h>

#define restrict __restrict__
extern "C"{
    #include <adt/Token.h>
    #include <string.h>
}
#undef restrict

SKYPAT_F(Token_unittest, Unknown)
{
    Token* token = new_UnknownToken();
    EXPECT_EQ(token->type, Token_Unknown);
    token->free(&token);
}

SKYPAT_F(Token_unittest, Keyword_Unknown)
{
    Token* token = new_KeywordToken(Keyword_Unknown);
    EXPECT_EQ(token->type, Token_Keyword);
    EXPECT_EQ(token->value.keyword, Keyword_Unknown);
    token->free(&token);
}

SKYPAT_F(Token_unittest, Keyword_auto)
{
    Token* token = new_KeywordToken(Keyword_auto);
    EXPECT_EQ(token->type, Keyword_auto);
    EXPECT_EQ(token->value.keyword, Keyword_auto);
    token->free(&token);
}

SKYPAT_F(Token_unittest, Identifier)
{
    char *identifier = (char*) calloc(5, sizeof(char));
    strcpy(identifier, "Test");
    Token* token = new_IdentifierToken(identifier);
    EXPECT_EQ(token->type, Token_Identifier);
    EXPECT_FALSE(strcmp(token->value.identifier, "Test"));
    token->free(&token);
}

SKYPAT_F(Token_unittest, Integer)
{
    Token* token = new_IntegerToken(1234);
    EXPECT_EQ(token->type, Token_Integer);
    EXPECT_EQ(token->value.integer, 1234);
    token->free(&token);
}

SKYPAT_F(Token_unittest, Floating)
{
    Token* token = new_FloatingToken(3.1415926);
    EXPECT_EQ(token->type, Token_Floating);
    EXPECT_EQ(token->value.floating, 3.1415926);
    token->free(&token);
}

SKYPAT_F(Token_unittest, Character)
{
    Token* token = new_CharacterToken('a');
    EXPECT_EQ(token->type, Token_Character);
    EXPECT_EQ(token->value.character, 'a');
    token->free(&token);
}

SKYPAT_F(Token_unittest, String)
{
    char *string = (char*) calloc(5, sizeof(char));
    strcpy(string, "TestTest");
    Token* token = new_StringToken(string);
    EXPECT_EQ(token->type, Token_String);
    EXPECT_FALSE(strcmp(token->value.string, "TestTest"));
    token->free(&token);
}

SKYPAT_F(Token_unittest, Punctuator)
{
    Token* token = new_PunctuatorToken(Punctuator_Unknown);
    EXPECT_EQ(token->type, Token_Punctuator);
    token->free(&token);
}