// Simple semantic pass for basic checks
#pragma once

#include "AST.hpp"
#include <memory>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <optional>
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
    // recorded canonical type representations for compatibility checks (structural)
    std::unordered_map<std::string, std::shared_ptr<TypeNode>> declaredTypeRepr{};
    // record the span of the first-seen declaration for better diagnostics
    std::unordered_map<std::string, wvmcc::SourceSpan> declaredSignatureSpan{};
    // ASTVisitor hooks
    void onIdent(const ASTVisitor::IdentifierExprPtr &id) override;
    void onFunctionDef(const FunctionDefPtr &f) override;
    void onStaticAssert(const ExternalDecl::StaticAssertPtr &sa) override;
    void onDeclaration(const DeclarationPtr &d) override;
    // function enter/exit hooks for block-scope checks
    void onEnterFunction(const FunctionDefPtr &f) override;
    void onExitFunction(const FunctionDefPtr &f) override;
    // internal (static) definitions tracking: name -> (span, definitive)
    std::unordered_map<std::string, std::pair<wvmcc::SourceSpan, bool>> internalDefs{};
    // pointers to StructOrUnionSpecifier objects already recorded as definitions
    // (prevents forward references via shared pointers from being flagged as duplicates)
    std::unordered_set<const void*> seenSuDefs_{};
    // recorded tag definitions: tag name -> span (for struct/union and enum definitions)
    std::unordered_map<std::string, wvmcc::SourceSpan> structUnionTagDefs{};
    std::unordered_map<std::string, wvmcc::SourceSpan> enumTagDefs{};
    // restrict associations: object name -> (restrict pointer name, span)
    std::unordered_map<std::string, std::pair<std::string, wvmcc::SourceSpan>> restrictAssoc{};
    // Function declaration summary per TU for inline rules
    struct FuncDeclInfo {
        int totalDecls = 0;
        int inlineDecls = 0;
        int externDecls = 0;
        int staticDecls = 0;
        bool hasDef = false;
        bool defIsInline = false;
    };
    std::unordered_map<std::string, FuncDeclInfo> functionDecls{};
    // names for which all file-scope declarations include inline (without extern)
    std::unordered_set<std::string> inlineOnlyNames{};
    // Alignment information: canonical text and optional parsed numeric value
    struct AlignInfo {
        std::string canon;
        std::optional<long long> value;
    };
    // alignment specifiers seen for declarations (name -> AlignInfo)
    std::unordered_map<std::string, AlignInfo> seenAlign{};
    // span where an alignment spec was first seen for a name
    std::unordered_map<std::string, wvmcc::SourceSpan> seenAlignSpan{};
    // alignment specifiers recorded for definitions (name -> AlignInfo; empty.canon means no align)
    std::unordered_map<std::string, AlignInfo> defAlign{};
    // span where a definition's alignment was recorded
    std::unordered_map<std::string, wvmcc::SourceSpan> defAlignSpan{};
    // current function nesting depth
    int functionDepth{0};
    // Compute alignment information from DeclarationSpecifiers (exprs and strings) with access to TU
    std::pair<std::optional<long long>, std::string> computeAlignFromSpecsTU(const DeclarationSpecifiers &specs) const;
    // structural comparison of TypeNode
    static bool typeNodesEqual(const std::shared_ptr<TypeNode> &a, const std::shared_ptr<TypeNode> &b);
    // Result of expression type analysis
    struct ExprTypeResult {
        std::shared_ptr<TypeNode> type;
        bool isLvalue{false};
        bool isFunctionDesignator{false};
        bool isVoid{false};
    };
public:
    // Compute the type and value category of an expression for semantic checks.
    ExprTypeResult typeOfExpr(const ExprPtr &e) const;
    // Build a TypeNode from a declaration-specifiers + declarator.
    // `inParamPrototype` indicates parameter prototype scope (affects VLA handling).
    // Public so static helper functions in Semantic.cpp can call it.
    std::shared_ptr<TypeNode> buildTypeFromDeclaration(const DeclarationSpecifiers &specs, const DeclaratorPtr &decl, bool inParamPrototype = false, bool *outVariablyModified = nullptr) const;
    // Produce a canonical type representation by resolving typedef-names via
    // the translation unit and expanding underlying type representations.
    // Public so codegen can resolve typedef-named parameter/local types to
    // their underlying struct/union (needed for member access).
    std::shared_ptr<TypeNode> canonicalTypeRepr(const DeclarationSpecifiers &specs, const DeclaratorPtr &decl) const;

    // Resolve a bare TypeNode (e.g. a `sizeof(type-name)` operand produced by
    // the parser) so struct/union tag-only references and typedef-names are
    // completed to their definitions. Without this, TypeMap sizes an
    // unresolved tag/typedef as 0. Returns a resolved copy (or the input if
    // already complete / nothing to resolve).
    std::shared_ptr<TypeNode> resolveTypeNode(const std::shared_ptr<TypeNode> &type) const;

private:
    // pointer to diagnostics vector during a run so hooks can append
    std::vector<wvmcc::Diagnostic> *curDiagnostics{nullptr};
};

} // namespace wvmcc::parser
