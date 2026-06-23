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
    // File-scope object names with at least one tentative definition (6.9.2):
    // an uninitialized, non-extern declaration. Tentative definitions collapse,
    // so they do not count toward the multiple-definitions check, but they DO
    // provide an external definition (so no "used but undefined" warning).
    std::unordered_set<std::string> tentativeDefs{};
    // File-scope names that decay to an address constant (arrays and functions),
    // collected up front so the static-initializer-constant check (which runs
    // before per-expression types are computed) can accept `static int *p = arr;`
    // and `static fn_t f = func;`.
    std::unordered_set<std::string> addressConstantNames_{};
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
    // full-expression and block-scope hooks (constraint diagnostics)
    void onExpr(const ExprPtr &e) override;
    void onStmt(const StmtPtr &s) override;
    void onEnterBlock() override;
    void onExitBlock() override;

    // Local (block-scope) symbol information used by typeOfExpr / constraint
    // checks. File-scope objects use declaredTypeRepr; block-scope objects are
    // not recorded there, so they are tracked here in a scope stack.
    struct LocalSym {
        std::shared_ptr<TypeNode> type; // canonical type of the object
        bool isConst{false};            // top-level const-qualified object
        bool isRegister{false};         // declared with `register` (6.7.1p6)
        wvmcc::SourceSpan span{};       // declaration span (for redefinition diag)
    };
    // Stack of block scopes (innermost last). Each maps name -> LocalSym.
    std::vector<std::unordered_map<std::string, LocalSym>> localScopes{};
    // Look up a name in the local scope stack (innermost first). Returns
    // nullptr if not found.
    const LocalSym *lookupLocal(const std::string &name) const;
    // Record a block-scope object in the current scope; returns false (and does
    // not overwrite) if the name already exists in the *current* scope.
    bool declareLocal(const std::string &name, const LocalSym &sym);
    // internal (static) definitions tracking: name -> (span, definitive)
    std::unordered_map<std::string, std::pair<wvmcc::SourceSpan, bool>> internalDefs{};
    // pointers to StructOrUnionSpecifier objects already recorded as definitions
    // (prevents forward references via shared pointers from being flagged as duplicates)
    std::unordered_set<const void*> seenSuDefs_{};
    // recorded tag definitions: tag name -> span (for struct/union and enum definitions)
    std::unordered_map<std::string, wvmcc::SourceSpan> structUnionTagDefs{};
    // File-scope internal-linkage (static) tentative definitions naming a
    // struct/union tag (C 6.9.2p3). The tag may be completed later in the TU, so
    // completeness is checked at end of translation unit (Semantic::run).
    struct PendingTentativeType {
        std::string tag;            // struct/union tag name
        bool isUnion{false};
        wvmcc::SourceSpan span{};
    };
    std::vector<PendingTentativeType> pendingStaticTentativeTypes_{};
    // True while traversing the body of an inline definition with external
    // linkage (declared `inline`, neither `static` nor `extern`). Such a
    // definition shall not define a modifiable static-duration object
    // (C 6.7.4p3).
    bool inInlineExternalDef_{false};
    std::unordered_map<std::string, wvmcc::SourceSpan> enumTagDefs{};
    // recorded tag *kind* per tag name for tag-kind-mismatch detection
    // (C 6.7.2.3p2): a tag may only be re-used with the same kind. Value is one
    // of 's' (struct), 'u' (union), 'e' (enum).
    std::unordered_map<std::string, char> tagKinds_{};
    // Scan declaration/function specifiers for struct/union/enum tag references
    // and emit a diagnostic when a tag is used with a different kind than it was
    // first introduced with.
    void checkTagKinds(const DeclarationSpecifiers &specs, const wvmcc::SourceSpan &span, std::vector<wvmcc::Diagnostic> &diagnostics);
    // Validate bit-field widths (C 6.7.2.1p4) in any struct/union specifier with
    // a body appearing in `specs` (recurses into nested aggregates and _Atomic).
    void checkBitfields(const DeclarationSpecifiers &specs, std::vector<wvmcc::Diagnostic> &diagnostics);
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
    // Result of expression type analysis
    struct ExprTypeResult {
        std::shared_ptr<TypeNode> type;
        bool isLvalue{false};
        bool isFunctionDesignator{false};
        bool isVoid{false};
        bool isConst{false};     // lvalue designates a const-qualified object
        bool isBitfield{false};  // lvalue designates a bit-field member (6.5.3.2p1)
        bool isRegister{false};  // lvalue designates a register-declared object
    };
public:
    // structural comparison of TypeNode
    static bool typeNodesEqual(const std::shared_ptr<TypeNode> &a, const std::shared_ptr<TypeNode> &b);
    // Compute the type and value category of an expression for semantic checks.
    ExprTypeResult typeOfExpr(const ExprPtr &e) const;
    // Whether an expression / initializer is a valid constant for an object with
    // static storage duration (6.6p7-9). Members (not free functions) so the
    // identifier case can consult typeOfExpr: a bare identifier naming an array
    // or function decays to an address constant, while a scalar object's value
    // is not constant.
    bool exprIsStaticInitConstant(const ExprPtr &e) const;
    bool initializerIsConstant(const InitializerPtr &init, std::vector<wvmcc::Diagnostic> &diagnostics) const;
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
