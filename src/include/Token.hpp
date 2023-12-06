#ifndef WVMCC_Token_DEF
#define WVMCC_Token_DEF

#include <iostream>
#include <variant>
#include <SourceFile.hpp>

namespace WasmVM {

namespace TokenType {

struct Punctuator {
    enum Type {
        Bracket_L, Bracket_R, Paren_L, Paren_R, Brace_L, Brace_R, Semi, Tlide, Query, Comma,
        Hash, DoubleHash, Dot, Ellipsis, Aster, AsterEqual, Caret, CaretEqual, Exclam, ExclamEqual, Equal, DoubleEqual, Slash, SlashEqual, Colon, Plus, DoublePlus, PlusEqual, Amp, DoubleAmp, AmpEqual, Bar, DoubleBar, BarEqual, Minus, DoubleMinus, MinusEqual, Arrow, Shift_R, Great, GreatEqual, ShiftEqual_R, Perce, PerceEqual, Shift_L, ShiftEqual_L, Less, LessEqual
    };
    Punctuator(Type t) : type(t){}
    Type type;
};

struct NewLine {};
struct Lparen {};

using Base = std::variant<Punctuator, NewLine, Lparen>;

} // namespace Token


struct Token : public TokenType::Base {

    template<typename T>
    Token(T t, SourcePos p = SourcePos()) : TokenType::Base(t), pos(p){}

    inline operator TokenType::Punctuator(){return std::get<TokenType::Punctuator>(*this);}
    inline operator TokenType::NewLine(){return std::get<TokenType::NewLine>(*this);}
    inline operator TokenType::Lparen(){return std::get<TokenType::Lparen>(*this);}
    SourcePos pos;
};

} // namespace WasmVM

std::ostream& operator<<(std::ostream&, WasmVM::Token&);

#endif