#ifndef WVMCC_Token_DEF
#define WVMCC_Token_DEF

#include <iostream>
#include <variant>
#include <type_traits>
#include <string>
#include <cstddef>
#include <SourceStream.hpp>

namespace WasmVM {

namespace TokenType {

struct Punctuator {
    enum Type {
        Bracket_L, Bracket_R, Paren_L, Paren_R, Brace_L, Brace_R, Semi, Tlide, Query, Comma,
        Hash, DoubleHash, Dot, Ellipsis, Aster, AsterEqual, Caret, CaretEqual, Exclam, ExclamEqual, Equal, DoubleEqual, Slash, SlashEqual, Colon, Plus, DoublePlus, PlusEqual, Amp, DoubleAmp, AmpEqual, Bar, DoubleBar, BarEqual, Minus, DoubleMinus, MinusEqual, Arrow, Shift_R, Great, GreatEqual, ShiftEqual_R, Perce, PerceEqual, Shift_L, ShiftEqual_L, Less, LessEqual
    };
    Punctuator(Type t) : type(t){}
    Type type;
    constexpr bool operator==(const Punctuator& op) const {
        return type == op.type;
    };
};

struct Keyword {
    Keyword(std::string);
    std::string value;

    static bool is_keyword(std::string);
    
    constexpr bool operator==(const Keyword& op) const {
        return value == op.value;
    };
};

struct NewLine {};
bool operator==(const NewLine&, const NewLine&);

struct WhiteSpace {};
bool operator==(const WhiteSpace&, const WhiteSpace&);

struct PPNumber {
    PPNumber(std::string s) : sequence(s){};
    template<typename T>
    T get();
    enum {Float, Int} type;
    std::string sequence;
private:
    int base();
};
bool operator==(const PPNumber&, const PPNumber&);

struct Identifier {
    Identifier(std::string s) : sequence(s){};
    std::string sequence;
};
bool operator==(const Identifier&, const Identifier&);

struct CharacterConstant {
    CharacterConstant(std::string sequence);
    std::string sequence;
    std::variant<int, wchar_t, char16_t, char32_t> value;
};
bool operator==(const CharacterConstant&, const CharacterConstant&);

struct StringLiteral {
    StringLiteral(std::string sequence);
    std::string sequence;
    std::variant<std::string, std::u8string, std::wstring, std::u16string, std::u32string> value;
    StringLiteral& operator+=(StringLiteral);
};
bool operator==(const StringLiteral&, const StringLiteral&);

struct HeaderName {
    HeaderName(std::string s) : sequence(s){};
    std::string sequence;
};
bool operator==(const HeaderName&, const HeaderName&);

using IntegerConstant = std::variant<intmax_t, uintmax_t>;
using FloatingConstant = long double;

using Base = std::variant<
    Punctuator,
    NewLine,
    WhiteSpace,
    PPNumber,
    Identifier,
    IntegerConstant,
    FloatingConstant,
    CharacterConstant,
    HeaderName,
    StringLiteral,
    Keyword
>;

template<typename T> requires
    std::is_same_v<T, Punctuator> ||
    std::is_same_v<T, NewLine> ||
    std::is_same_v<T, WhiteSpace> ||
    std::is_same_v<T, PPNumber> ||
    std::is_same_v<T, Identifier> ||
    std::is_same_v<T, IntegerConstant> ||
    std::is_same_v<T, FloatingConstant> ||
    std::is_same_v<T, CharacterConstant> ||
    std::is_same_v<T, HeaderName> ||
    std::is_same_v<T, StringLiteral> ||
    std::is_same_v<T, Keyword>
struct is_valid {
    template<typename U> requires std::is_constructible_v<T, U>
    is_valid(){}
    constexpr static bool value = true;
};

} // namespace TokenType

struct Token : public TokenType::Base {

    template<typename T> requires TokenType::is_valid<T>::value || std::is_constructible_v<TokenType::is_valid<T>> 
    Token(T t, SourcePos p = SourcePos()) : TokenType::Base(t), pos(p){}

    template<typename T> requires TokenType::is_valid<T>::value
    inline operator T(){
        return std::get<T>(*this);
    }

    std::string str();

    SourcePos pos;
};

} // namespace WasmVM

std::ostream& operator<<(std::ostream&, WasmVM::Token&);
std::ostream& operator<<(std::ostream&, WasmVM::TokenType::Punctuator&);
std::ostream& operator<<(std::ostream&, WasmVM::TokenType::Keyword&);
std::ostream& operator<<(std::ostream&, WasmVM::TokenType::Identifier&);
std::ostream& operator<<(std::ostream&, WasmVM::TokenType::IntegerConstant&);
std::ostream& operator<<(std::ostream&, WasmVM::TokenType::CharacterConstant&);
std::ostream& operator<<(std::ostream&, WasmVM::TokenType::StringLiteral&);

#endif