#include <Lexer.h>

Token* lex_Keyword(char **inputPtr){
    char *cursor = *inputPtr;
    while(*cursor == '_' || (*cursor >= 'a' && *cursor <= 'z') || (*cursor >= 'A' && *cursor <= 'Z')){
        ++cursor;
    }
    size_t length = cursor - *inputPtr;
    if(length){
        if(!strncmp(*inputPtr, "auto", length)){
            *inputPtr += strlen("auto");
            return new_KeywordToken(Keyword_auto);
        }else if(!strncmp(*inputPtr, "break", length)){
            *inputPtr += strlen("break");
            return new_KeywordToken(Keyword_break);
        }else if(!strncmp(*inputPtr, "case", length)){
            *inputPtr += strlen("case");
            return new_KeywordToken(Keyword_case);
        }else if(!strncmp(*inputPtr, "char", length)){
            *inputPtr += strlen("char");
            return new_KeywordToken(Keyword_char);
        }else if(!strncmp(*inputPtr, "const", length)){
            *inputPtr += strlen("const");
            return new_KeywordToken(Keyword_const);
        }else if(!strncmp(*inputPtr, "continue", length)){
            *inputPtr += strlen("continue");
            return new_KeywordToken(Keyword_continue);
        }else if(!strncmp(*inputPtr, "default", length)){
            *inputPtr += strlen("default");
            return new_KeywordToken(Keyword_default);
        }else if(!strncmp(*inputPtr, "do", length)){
            *inputPtr += strlen("do");
            return new_KeywordToken(Keyword_do);
        }else if(!strncmp(*inputPtr, "double", length)){
            *inputPtr += strlen("double");
            return new_KeywordToken(Keyword_double);
        }else if(!strncmp(*inputPtr, "else", length)){
            *inputPtr += strlen("else");
            return new_KeywordToken(Keyword_else);
        }else if(!strncmp(*inputPtr, "enum", length)){
            *inputPtr += strlen("enum");
            return new_KeywordToken(Keyword_enum);
        }else if(!strncmp(*inputPtr, "extern", length)){
            *inputPtr += strlen("extern");
            return new_KeywordToken(Keyword_extern);
        }else if(!strncmp(*inputPtr, "float", length)){
            *inputPtr += strlen("float");
            return new_KeywordToken(Keyword_float);
        }else if(!strncmp(*inputPtr, "for", length)){
            *inputPtr += strlen("for");
            return new_KeywordToken(Keyword_for);
        }else if(!strncmp(*inputPtr, "goto", length)){
            *inputPtr += strlen("goto");
            return new_KeywordToken(Keyword_goto);
        }else if(!strncmp(*inputPtr, "if", length)){
            *inputPtr += strlen("if");
            return new_KeywordToken(Keyword_if);
        }else if(!strncmp(*inputPtr, "inline", length)){
            *inputPtr += strlen("inlien");
            return new_KeywordToken(Keyword_inline);
        }else if(!strncmp(*inputPtr, "int", length)){
            *inputPtr += strlen("int");
            return new_KeywordToken(Keyword_int);
        }else if(!strncmp(*inputPtr, "long", length)){
            *inputPtr += strlen("long");
            return new_KeywordToken(Keyword_long);
        }else if(!strncmp(*inputPtr, "register", length)){
            *inputPtr += strlen("register");
            return new_KeywordToken(Keyword_register);
        }else if(!strncmp(*inputPtr, "restrict", length)){
            *inputPtr += strlen("restrict");
            return new_KeywordToken(Keyword_restrict);
        }else if(!strncmp(*inputPtr, "return", length)){
            *inputPtr += strlen("return");
            return new_KeywordToken(Keyword_return);
        }else if(!strncmp(*inputPtr, "short", length)){
            *inputPtr += strlen("short");
            return new_KeywordToken(Keyword_short);
        }else if(!strncmp(*inputPtr, "signed", length)){
            *inputPtr += strlen("signed");
            return new_KeywordToken(Keyword_signed);
        }else if(!strncmp(*inputPtr, "sizeof", length)){
            *inputPtr += strlen("sizeof");
            return new_KeywordToken(Keyword_sizeof);
        }else if(!strncmp(*inputPtr, "static", length)){
            *inputPtr += strlen("static");
            return new_KeywordToken(Keyword_static);
        }else if(!strncmp(*inputPtr, "struct", length)){
            *inputPtr += strlen("struct");
            return new_KeywordToken(Keyword_struct);
        }else if(!strncmp(*inputPtr, "switch", length)){
            *inputPtr += strlen("switch");
            return new_KeywordToken(Keyword_switch);
        }else if(!strncmp(*inputPtr, "typedef", length)){
            *inputPtr += strlen("typedef");
            return new_KeywordToken(Keyword_typedef);
        }else if(!strncmp(*inputPtr, "union", length)){
            *inputPtr += strlen("union");
            return new_KeywordToken(Keyword_union);
        }else if(!strncmp(*inputPtr, "unsigned", length)){
            *inputPtr += strlen("unsigned");
            return new_KeywordToken(Keyword_unsigned);
        }else if(!strncmp(*inputPtr, "void", length)){
            *inputPtr += strlen("void");
            return new_KeywordToken(Keyword_void);
        }else if(!strncmp(*inputPtr, "volatile", length)){
            *inputPtr += strlen("volatile");
            return new_KeywordToken(Keyword_volatile);
        }else if(!strncmp(*inputPtr, "while", length)){
            *inputPtr += strlen("while");
            return new_KeywordToken(Keyword_while);
        }else if(!strncmp(*inputPtr, "_Alignas", length)){
            *inputPtr += strlen("_Alignas");
            return new_KeywordToken(Keyword_Alignas);
        }else if(!strncmp(*inputPtr, "_Alignof", length)){
            *inputPtr += strlen("_Alignof");
            return new_KeywordToken(Keyword_Alignof);
        }else if(!strncmp(*inputPtr, "_Atomic", length)){
            *inputPtr += strlen("_Atomic");
            return new_KeywordToken(Keyword_Atomic);
        }else if(!strncmp(*inputPtr, "_Bool", length)){
            *inputPtr += strlen("_Bool");
            return new_KeywordToken(Keyword_Bool);
        }else if(!strncmp(*inputPtr, "_Complex", length)){
            *inputPtr += strlen("_Complex");
            return new_KeywordToken(Keyword_Complex);
        }else if(!strncmp(*inputPtr, "_Generic", length)){
            *inputPtr += strlen("_Generic");
            return new_KeywordToken(Keyword_Generic);
        }else if(!strncmp(*inputPtr, "_Imaginary", length)){
            *inputPtr += strlen("_Imaginary");
            return new_KeywordToken(Keyword_Imaginary);
        }else if(!strncmp(*inputPtr, "_Noreturn", length)){
            *inputPtr += strlen("_Noreturn");
            return new_KeywordToken(Keyword_Noreturn);
        }else if(!strncmp(*inputPtr, "_Static_assert", length)){
            *inputPtr += strlen("_Static_assert");
            return new_KeywordToken(Keyword_Static_assert);
        }else if(!strncmp(*inputPtr, "_Thread_local", length)){
            *inputPtr += strlen("_Thread_local");
            return new_KeywordToken(Keyword_Thread_local);
        }
    }
    return NULL;
}