#include <Lexer.h>

Token* lex_Punctuator(char **inputPtr){
    char* cursor = *inputPtr;
    if( (cursor[0] == '#' && cursor[1] == '#') ||
        (cursor[0] == '%' && cursor[1] == ':' && cursor[2] == '%' && cursor[3] == ':')
    ){
        if(cursor[0] == '#'){
            *inputPtr += 2;
        }else{
            *inputPtr += 4;
        }
        return new_PunctuatorToken(Punctuator_HashHash);
    }else if(cursor[0] == '.' && cursor[1] == '.' && cursor[2] == '.'){
        *inputPtr += 3;
        return new_PunctuatorToken(Punctuator_Ellipsis);
    }else if(cursor[0] == '<' && cursor[1] == '<' && cursor[2] == '='){
        *inputPtr += 3;
        return new_PunctuatorToken(Punctuator_Assign_L_shift);
    }else if(cursor[0] == '>' && cursor[1] == '>' && cursor[2] == '='){
        *inputPtr += 3;
        return new_PunctuatorToken(Punctuator_Assign_R_shift);
    }else if(cursor[0] == '-' && cursor[1] == '>'){
        *inputPtr += 2;
        return new_PunctuatorToken(Punctuator_Arrow);
    }else if(cursor[0] == '+' && cursor[1] == '+'){
        *inputPtr += 2;
        return new_PunctuatorToken(Punctuator_Increase);
    }else if(cursor[0] == '-' && cursor[1] == '-'){
        *inputPtr += 2;
        return new_PunctuatorToken(Punctuator_Decrease);
    }else if(cursor[0] == '<' && cursor[1] == '<'){
        *inputPtr += 2;
        return new_PunctuatorToken(Punctuator_L_Shift);
    }else if(cursor[0] == '>' && cursor[1] == '>'){
        *inputPtr += 2;
        return new_PunctuatorToken(Punctuator_R_Shift);
    }else if(cursor[0] == '<' && cursor[1] == '='){
        *inputPtr += 2;
        return new_PunctuatorToken(Punctuator_Less_than);
    }else if(cursor[0] == '>' && cursor[1] == '='){
        *inputPtr += 2;
        return new_PunctuatorToken(Punctuator_More_than);
    }else if(cursor[0] == '=' && cursor[1] == '='){
        *inputPtr += 2;
        return new_PunctuatorToken(Punctuator_Equal);
    }else if(cursor[0] == '!' && cursor[1] == '='){
        *inputPtr += 2;
        return new_PunctuatorToken(Punctuator_Not_equal);
    }else if(cursor[0] == '&' && cursor[1] == '&'){
        *inputPtr += 2;
        return new_PunctuatorToken(Punctuator_And);
    }else if(cursor[0] == '|' && cursor[1] == '|'){
        *inputPtr += 2;
        return new_PunctuatorToken(Punctuator_Or);
    }else if(cursor[0] == '*' && cursor[1] == '='){
        *inputPtr += 2;
        return new_PunctuatorToken(Punctuator_Assign_multiply);
    }else if(cursor[0] == '/' && cursor[1] == '='){
        *inputPtr += 2;
        return new_PunctuatorToken(Punctuator_Assign_division);
    }else if(cursor[0] == '%' && cursor[1] == '='){
        *inputPtr += 2;
        return new_PunctuatorToken(Punctuator_Assign_modulo);
    }else if(cursor[0] == '+' && cursor[1] == '='){
        *inputPtr += 2;
        return new_PunctuatorToken(Punctuator_Assign_addition);
    }else if(cursor[0] == '-' && cursor[1] == '='){
        *inputPtr += 2;
        return new_PunctuatorToken(Punctuator_Assign_substract);
    }else if(cursor[0] == '&' && cursor[1] == '='){
        *inputPtr += 2;
        return new_PunctuatorToken(Punctuator_Assign_and);
    }else if(cursor[0] == '^' && cursor[1] == '='){
        *inputPtr += 2;
        return new_PunctuatorToken(Punctuator_Assign_xor);
    }else if(cursor[0] == '|' && cursor[1] == '='){
        *inputPtr += 2;
        return new_PunctuatorToken(Punctuator_Assign_or);
    }else if(cursor[0] == '#' || (cursor[0] == '%' && cursor[0] == ':')){
        if(cursor[0] == '#'){
            *inputPtr += 1;
        }else{
            *inputPtr += 2;
        }
        return new_PunctuatorToken(Punctuator_Hash);
    }else if(cursor[0] == '['){
        *inputPtr += 1;
        return new_PunctuatorToken(Punctuator_L_Bracket);
    }else if(cursor[0] == ']'){
        *inputPtr += 1;
        return new_PunctuatorToken(Punctuator_R_Bracket);
    }else if(cursor[0] == '('){
        *inputPtr += 1;
        return new_PunctuatorToken(Punctuator_L_Parenthesis);
    }else if(cursor[0] == ')'){
        *inputPtr += 1;
        return new_PunctuatorToken(Punctuator_R_Paranthesis);
    }else if(cursor[0] == '{'){
        *inputPtr += 1;
        return new_PunctuatorToken(Punctuator_L_Brace);
    }else if(cursor[0] == '}'){
        *inputPtr += 1;
        return new_PunctuatorToken(Punctuator_R_Brace);
    }else if(cursor[0] == '.'){
        *inputPtr += 1;
        return new_PunctuatorToken(Punctuator_Period);
    }else if(cursor[0] == '&'){
        *inputPtr += 1;
        return new_PunctuatorToken(Punctuator_Amprecent);
    }else if(cursor[0] == '*'){
        *inputPtr += 1;
        return new_PunctuatorToken(Punctuator_Asterisk);
    }else if(cursor[0] == '+'){
        *inputPtr += 1;
        return new_PunctuatorToken(Punctuator_Plus);
    }else if(cursor[0] == '-'){
        *inputPtr += 1;
        return new_PunctuatorToken(Punctuator_Minus);
    }else if(cursor[0] == '~'){
        *inputPtr += 1;
        return new_PunctuatorToken(Punctuator_Tlide);
    }else if(cursor[0] == '!'){
        *inputPtr += 1;
        return new_PunctuatorToken(Punctuator_Exclamation);
    }else if(cursor[0] == '/'){
        *inputPtr += 1;
        return new_PunctuatorToken(Punctuator_Slash);
    }else if(cursor[0] == '%'){
        *inputPtr += 1;
        return new_PunctuatorToken(Punctuator_Percent);
    }else if(cursor[0] == '<'){
        *inputPtr += 1;
        return new_PunctuatorToken(Punctuator_Lesser);
    }else if(cursor[0] == '>'){
        *inputPtr += 1;
        return new_PunctuatorToken(Punctuator_Greater);
    }else if(cursor[0] == '^'){
        *inputPtr += 1;
        return new_PunctuatorToken(Punctuator_Caret);
    }else if(cursor[0] == '|'){
        *inputPtr += 1;
        return new_PunctuatorToken(Punctuator_Vertical_bar);
    }else if(cursor[0] == '?'){
        *inputPtr += 1;
        return new_PunctuatorToken(Punctuator_Query);
    }else if(cursor[0] == ':'){
        *inputPtr += 1;
        return new_PunctuatorToken(Punctuator_Colon);
    }else if(cursor[0] == ';'){
        *inputPtr += 1;
        return new_PunctuatorToken(Punctuator_Semicolon);
    }else if(cursor[0] == '='){
        *inputPtr += 1;
        return new_PunctuatorToken(Punctuator_Assign);
    }else if(cursor[0] == ','){
        *inputPtr += 1;
        return new_PunctuatorToken(Punctuator_Comma);
    }
    return NULL;
}