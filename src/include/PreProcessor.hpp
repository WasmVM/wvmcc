#ifndef WVMCC_PreProcessor_DEF
#define WVMCC_PreProcessor_DEF

#include <ostream>
#include <filesystem>
#include <stack>
#include <deque>
#include <string>
#include <optional>
#include <unordered_map>
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

    struct Macro {
        std::string name;
        std::optional<std::vector<std::string>> params;
        std::vector<Token> replacement;
        bool operator==(const Macro& op) const;
    };

    PreProcessor(std::filesystem::path filepath);
    std::optional<Token> get();

protected:
    struct Visitor;
    std::stack<std::shared_ptr<Visitor>> visitors;
    std::deque<Token> buffer;
    std::unordered_map<std::string, Macro> macros;

    friend struct Visitor;
};

} // namespace WasmVM

#endif