// Simple semantic pass for basic checks
#pragma once

#include "AST.hpp"
#include <memory>
#include <vector>
#include "../common.hpp"

namespace wvmcc::parser {

class Semantic {
public:
    explicit Semantic(bool verbose = false) : verbose_(verbose) {}

    // Run semantic checks on the translation unit and append diagnostics
    void run(const TranslationUnitPtr &tu, std::vector<wvmcc::Diagnostic> &diagnostics);

private:
    void checkExternal(const ExternalDeclPtr &e, std::vector<wvmcc::Diagnostic> &diagnostics);
    void checkDeclaration(const DeclarationPtr &d, std::vector<wvmcc::Diagnostic> &diagnostics);
    void checkFunction(const FunctionDefPtr &f, std::vector<wvmcc::Diagnostic> &diagnostics);

    bool verbose_ = false;
};

} // namespace wvmcc::parser
