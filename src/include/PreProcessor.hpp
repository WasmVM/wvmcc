#ifndef WVMCC_PreProcessor_DEF
#define WVMCC_PreProcessor_DEF

#include <istream>
#include <sstream>
#include <filesystem>
#include <stack>
#include <deque>
#include <string>
#include <optional>
#include <common.hpp>

namespace WasmVM {

struct PreProcessor {

    struct Token {
        enum {
            HeaderName, Identifier, PPNumber, CharConst, StringLiteral, Punctuator, WhiteSpace, NewLine
        } type;
        std::string text;
        SourcePos pos;
    };

    PreProcessor(std::filesystem::path filepath);
    std::optional<Token> get();

protected:
    struct Visitor;
    std::stack<std::shared_ptr<Visitor>> visitors;
    std::deque<Token> buffer;
};

} // namespace WasmVM

#endif