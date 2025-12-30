// Simple semantic pass for basic checks
#pragma once

#include "AST.hpp"
#include <memory>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include "../common.hpp"
#include "ASTVisitor.hpp"

namespace wvmcc::parser {

class Semantic : public ASTVisitor {
public:
    explicit Semantic(const TranslationUnitPtr &tu, bool verbose = false) : verbose_(verbose), tu_(tu) {}

    // Run semantic checks and append diagnostics. Returns true if semantic
    // pass produced no error diagnostics, false otherwise.
    bool run(std::vector<wvmcc::Diagnostic> &diagnostics);

private:
    void checkExternal(const ExternalDeclPtr &e, std::vector<wvmcc::Diagnostic> &diagnostics);
    void checkDeclaration(const DeclarationPtr &d, std::vector<wvmcc::Diagnostic> &diagnostics);
    void checkFunction(const FunctionDefPtr &f, std::vector<wvmcc::Diagnostic> &diagnostics);
    void recordDef(const std::string &name, const wvmcc::SourceSpan &span);

    bool verbose_ = false;
    TranslationUnitPtr tu_{};
    std::unordered_map<std::string, int> defCount{};
    std::unordered_map<std::string, wvmcc::SourceSpan> firstDefSpan{};
    std::unordered_set<std::string> usedNames{};
    // recorded declaration signatures for compatibility checks
    std::unordered_map<std::string, std::string> declaredSignatures{};
    // ASTVisitor hooks
    void onIdent(const ASTVisitor::IdentifierExprPtr &id) override;
    void onFunctionDef(const FunctionDefPtr &f) override;
    void onDeclaration(const DeclarationPtr &d) override;
    // function enter/exit hooks for block-scope checks
    void onEnterFunction(const FunctionDefPtr &f) override;
    void onExitFunction(const FunctionDefPtr &f) override;
    // internal (static) definitions tracking: name -> (span, definitive)
    std::unordered_map<std::string, std::pair<wvmcc::SourceSpan, bool>> internalDefs{};
    // current function nesting depth
    int functionDepth{0};
    // pointer to diagnostics vector during a run so hooks can append
    std::vector<wvmcc::Diagnostic> *curDiagnostics{nullptr};
};

} // namespace wvmcc::parser
