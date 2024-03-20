#ifndef WVMCC_PreProcessor_DEF
#define WVMCC_PreProcessor_DEF

#include <filesystem>
#include <stack>
#include <string>
#include <vector>
#include <list>
#include <type_traits>
#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <iterator>
#include <optional>
#include <Token.hpp>
#include <SourceStream.hpp>
#include <Util.hpp>

namespace WasmVM {

struct PreProcessor {

    using StreamType = std::istream;
    using Stream = SourceStreamBase<StreamType::int_type, StreamType::char_type>;

    struct PPToken : public std::optional<Token> {

        PPToken(std::nullopt_t n = std::nullopt) : std::optional<Token>(){}
        PPToken(Token&& token) : std::optional<Token>(token){}
        PPToken(Token& token) : std::optional<Token>(token){}

        template<typename T> requires TokenType::is_valid<T>::value
        inline bool hold() {
            return has_value() && std::holds_alternative<T>(value());
        }

        std::unordered_set<std::string> expanded;
        bool skipped = false;
    };

    PreProcessor(std::filesystem::path path);
    PreProcessor(std::filesystem::path path, std::string source);
    PPToken get();

#ifndef CCTEST
private:
#endif

    struct Line : public std::list<PPToken> {
        enum {None, Text, Directive} type;
    };

    struct Scanner {
        Scanner(Stream* stream);
        PPToken get();
    private:
        Stream* stream;
        enum class LineState : int{
            unknown = 0,
            normal = 1,
            hashed = 2,
            include = 3,
        } state;
    };


    struct Macro {
        std::string name;
        std::optional<std::vector<std::string>> params;
        std::vector<PPToken> replacement;
        bool operator==(const Macro& op) const;
    };

    struct Expression {
        Expression(Line::iterator cur, Line::iterator end);
        using Result = std::variant<intmax_t, uintmax_t, double>;
        Result eval();
    private:
        Line::iterator cur;
        Line::iterator end;
        Result primary();
        Result unary();
        Result multiplicative();
        Result additive();
        Result shift();
        Result relational();
        Result equality();
        Result bitwise_AND(); // TODO:
        Result bitwise_exclusive_OR(); // TODO:
        Result bitwise_inclusive_OR(); // TODO:
        Result logical_AND(); // TODO:
        Result logical_OR(); // TODO:
        Result conditional(); // TODO:

        template<typename T = void>
        struct lshift {
            template<typename U>
            U operator()(U& lhs, U& rhs){
                return lhs << rhs;
            }
        };
        template<typename T = void>
        struct rshift{
            template<typename U>
            U operator()(U& lhs, U& rhs){
                return lhs >> rhs;
            }
        };

        template<typename T>
        static Result& cast_result(Result& res);

        template<template<typename T> class Op, typename T = void>
        static Result binary_op(Result&, Result&);
        template<template<typename T> class Op, typename T = void>
            requires std::is_same_v<Op<T>, std::modulus<T>>
                || std::is_same_v<Op<T>, lshift<T>>
                || std::is_same_v<Op<T>, rshift<T>>
        static Result binary_op(Result&, Result&);
        template<template<typename T> class Op, typename T = void>
        static Result relation_op(Result&, Result&);

        static void implicit_cast(Result&, Result&);
    };

    std::stack<std::unique_ptr<Stream>> streams;
    Line line;
    std::unordered_map<std::string, Macro> macros = {};
    ssize_t if_level = 0;

    static Line::iterator skip_whitespace(Line::iterator it, Line::iterator end);
    void replace_macro(Line& line, std::unordered_map<std::string, Macro> macro_map, bool keep_defined = false);
    bool evaluate_condition();
    bool readline();
    void defined_operator();
    void jump_else();

    void define_directive();
    void if_directive();
    void ifdef_ifndef_directive(bool if_def = true);
    void else_directive();
    // void undef_directive(PPToken& token); // TODO:
    // void pragma_directive(PPToken& token); // TODO:
    // void include_directive(PPToken& token); // TODO:
    // void line_directive(PPToken& token); // TODO:
    // void error_directive(PPToken& token);
};

} // namespace WasmVM

#endif