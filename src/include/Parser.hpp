#ifndef WVMCC_Parser_DEF
#define WVMCC_Parser_DEF

#include <optional>
#include <Lexer.hpp>
#include <AST/Expr.hpp>

namespace WasmVM {

struct Parser {
    Parser(Lexer &lexer);

    template<typename T>
    std::optional<T> parse();

private:
    Lexer &lexer;
};

} // namespace WasmVM

#endif