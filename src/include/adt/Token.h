#ifndef WVMCC_TOKEN_DEF
#define WVMCC_TOKEN_DEF

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
    Token_Unknown = 0,
    Token_Keyword,
    Token_Identifier,
    Token_Integer,
    Token_Floating,
    Token_Character,
    Token_String,
    Token_Punctuator
} TokenType;

typedef enum {
    Keyword_Unknown = 0,
    Keyword_auto,
    Keyword_break,
    Keyword_case,
    Keyword_char,
    Keyword_const,
    Keyword_continue,
    Keyword_default,
    Keyword_do,
    Keyword_double,
    Keyword_else,
    Keyword_enum,
    Keyword_extern,
    Keyword_float,
    Keyword_for,
    Keyword_goto,
    Keyword_if,
    Keyword_inline,
    Keyword_int,
    Keyword_long,
    Keyword_register,
    Keyword_restrict,
    Keyword_return,
    Keyword_short,
    Keyword_signed,
    Keyword_sizeof,
    Keyword_static,
    Keyword_struct,
    Keyword_switch,
    Keyword_typedef,
    Keyword_union,
    Keyword_unsigned,
    Keyword_void,
    Keyword_volatile,
    Keyword_while,
    Keyword_Alignas,
    Keyword_Alignof,
    Keyword_Atomic,
    Keyword_Bool,
    Keyword_Complex,
    Keyword_Generic,
    Keyword_Imaginary,
    Keyword_Noreturn,
    Keyword_Static_assert,
    Keyword_Thread_local
} Keyword;

typedef enum {
    Punctuator_Unknown = 0
} Punctuator;

typedef struct _Token{
    TokenType type;
    union {
        double floating;
        unsigned long long int integer;
        uint32_t character;
        const uint32_t *string;
        const char *identifier;
        Keyword keyword;
        Punctuator punctuator;
    } value;
    unsigned int byteSize;
    _Bool isUnsigned;
    void (*free)(struct _Token** token);
} Token;

Token* new_UnknownToken();
Token* new_KeywordToken(const Keyword keyword);
Token* new_IdentifierToken(const char *identifier);
Token* new_IntegerToken(const unsigned long long int value, const unsigned int byteSize, const _Bool isUnsigned);
Token* new_FloatingToken(const double value, const unsigned int byteSize);
Token* new_CharacterToken(const uint32_t value, const unsigned int byteSize);
Token* new_StringToken(const uint32_t *string, const unsigned int byteSize);
Token* new_PunctuatorToken(const Punctuator punctuator);

#endif