#include "Semantic.hpp"

#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <string>
#include <algorithm>
#include "ConstExprEval.hpp"

using wvmcc::Diagnostic;


namespace wvmcc::parser {
// Try to parse an alignment expression string to a numeric value.
// Supports simple integer literals (decimal) and returns std::nullopt for others.
static std::optional<long long> parseAlignmentValue(const std::string &s) {
    // trim
    size_t a = 0; while (a < s.size() && isspace((unsigned char)s[a])) a++;
    size_t b = s.size(); while (b > a && isspace((unsigned char)s[b-1])) b--;
    if (b <= a) return std::nullopt;
    std::string t = s.substr(a, b - a);
    // If it's a simple decimal integer literal
    bool allDigits = true;
    for (char c : t) { if (!isdigit((unsigned char)c)) { allDigits = false; break; } }
    if (allDigits) {
        try {
            long long v = std::stoll(t);
            return v;
        } catch (...) { return std::nullopt; }
    }
    // Could add support for more forms (_Alignof(...)) in future
    // Support simple _Alignof(type-name) forms by mapping common builtins
    // to target alignment values (assume typical LP64 layout: char=1, short=2,
    // int=4, long=8, long long=8, float=4, double=8, pointer=8).
    // Accept forms like "_Alignof(int)" or "_Alignof( unsigned long )".
    auto starts_with = [&](const std::string &p) {
        if (t.size() < p.size()) return false;
        return t.compare(0, p.size(), p) == 0;
    };
    // find _Alignof(...)
    const std::string key = "_Alignof";
    size_t pos = std::string::npos;
    for (size_t i = 0; i + key.size() <= t.size(); ++i) {
        if (t.compare(i, key.size(), key) == 0) { pos = i; break; }
    }
    if (pos != std::string::npos) {
        // find '(' after key
        size_t p = t.find('(', pos + key.size());
        if (p == std::string::npos) return std::nullopt;
        size_t q = t.find(')', p+1);
        if (q == std::string::npos) return std::nullopt;
        std::string inner = t.substr(p+1, q - (p+1));
        // trim inner
        size_t ia = 0; while (ia < inner.size() && isspace((unsigned char)inner[ia])) ia++;
        size_t ib = inner.size(); while (ib > ia && isspace((unsigned char)inner[ib-1])) ib--;
        if (ib <= ia) return std::nullopt;
        std::string typ = inner.substr(ia, ib-ia);
        // normalize multiple spaces to single and lower-case
        std::string norm;
        bool lastSpace = false;
        for (char c : typ) {
            if (isspace((unsigned char)c)) {
                if (!lastSpace) { norm.push_back(' '); lastSpace = true; }
            } else { norm.push_back((char)tolower((unsigned char)c)); lastSpace = false; }
        }
        // simple mapping for common type-names
        static const std::unordered_map<std::string,long long> alignMap = {
            {"char",1}, {"signed char",1}, {"unsigned char",1},
            {"short",2}, {"short int",2}, {"signed short",2}, {"signed short int",2}, {"unsigned short",2}, {"unsigned short int",2},
            {"int",4}, {"signed",4}, {"signed int",4}, {"unsigned",4}, {"unsigned int",4},
            {"long",8}, {"long int",8}, {"signed long",8}, {"signed long int",8}, {"unsigned long",8}, {"unsigned long int",8},
            {"long long",8}, {"long long int",8}, {"unsigned long long",8}, {"unsigned long long int",8},
            {"float",4}, {"double",8}, {"long double",16}, {"_bool",1},
            {"void *",8}, {"char *",8}
        };
        // If the normalized type is a pointer-like form ending with '*', treat as pointer
        if (!norm.empty() && norm.back() == '*') return 8;
        auto it = alignMap.find(norm);
        if (it != alignMap.end()) return it->second;
        return std::nullopt;
    }
    return std::nullopt;
}

// Compute effective alignment value from a vector of align-spec strings.
// Returns optional numeric value (max of numeric entries) and also returns
// a canonical concatenated text for fallback comparisons.
static std::pair<std::optional<long long>, std::string> computeAlignFromSpecs(const std::vector<std::string> &specs) {
    std::optional<long long> maxv;
    std::string canon;
    for (const auto &a : specs) {
        if (!canon.empty()) canon += ";";
        // trim each element
        size_t i = 0; while (i < a.size() && isspace((unsigned char)a[i])) i++;
        size_t j = a.size(); while (j > i && isspace((unsigned char)a[j-1])) j--;
        std::string t = a.substr(i, j - i);
        canon += t;
        auto val = parseAlignmentValue(t);
        if (val.has_value()) {
            if (!maxv.has_value() || *val > *maxv) maxv = val;
        }
    }
    return {maxv, canon};
}

// forward declarations for helper functions defined later in this file
static std::string declaratorName(const DeclaratorPtr &d);
static bool declarationObjectIsConst(const DeclarationPtr &d);
static bool specsDeclObjectIsConst(const DeclarationSpecifiers &specs, const DeclaratorPtr &declarator);


// Member-level implementation that can resolve _Alignof(type-name) via the
// translation unit when possible. It delegates to the static computeAlignFromSpecs
// for basic canonicalization, but attempts to evaluate _Alignof(...) expressions
// that name structs/unions/typedefs by walking the TU.
std::pair<std::optional<long long>, std::string> Semantic::computeAlignFromSpecsTU(const DeclarationSpecifiers &specs) const {
    std::optional<long long> maxv;
    std::string canon;
    auto trim = [](const std::string &s) {
        size_t a = 0; while (a < s.size() && isspace((unsigned char)s[a])) a++;
        size_t b = s.size(); while (b > a && isspace((unsigned char)s[b-1])) b--;
        return s.substr(a, b - a);
    };
    // helper to compute struct/union alignment from its specifier (recursive)
    std::function<long long(const std::shared_ptr<StructOrUnionSpecifier>&)> computeAlignForSU;
    computeAlignForSU = [&](const std::shared_ptr<StructOrUnionSpecifier> &su)->long long {
        long long amax = 1;
        if (!su) return amax;
        for (const auto &m : su->members) {
            if (m.declarators.empty()) {
                // anonymous nested specifiers: try nested structs/unions
                for (const auto &nts : m.specifiers.typeSpecifiers) {
                    if (nts.kind == DeclarationSpecifiers::TypeSpecifier::Kind::StructOrUnion && nts.su && nts.su->hasBody) {
                        long long sub = computeAlignForSU(nts.su);
                        if (sub > amax) amax = sub;
                    }
                }
                continue;
            }
            for (const auto &sd : m.declarators) {
                bool isPtr = false;
                if (sd.declarator) {
                    DeclaratorPtr cur = sd.declarator;
                    while (cur) {
                        if (cur->kind == Declarator::Kind::Pointer) { isPtr = true; break; }
                        if (cur->inner.has_value()) cur = cur->inner.value(); else break;
                    }
                }
                if (isPtr) { if (8 > amax) amax = 8; continue; }
                // look at the specifiers for a simple mapping
                for (const auto &mts : m.specifiers.typeSpecifiers) {
                    if (mts.kind == DeclarationSpecifiers::TypeSpecifier::Kind::Simple) {
                        for (auto st : mts.simple) {
                            long long v = 4;
                            using S = DeclarationSpecifiers::SimpleTypeSpecifier;
                            if (st == S::Char) v = 1;
                            else if (st == S::Short) v = 2;
                            else if (st == S::Int) v = 4;
                            else if (st == S::Long) v = 8;
                            else if (st == S::Float) v = 4;
                            else if (st == S::Double) v = 8;
                            if (v > amax) amax = v;
                        }
                    } else if (mts.kind == DeclarationSpecifiers::TypeSpecifier::Kind::StructOrUnion && mts.su && mts.su->hasBody) {
                        long long sub = computeAlignForSU(mts.su);
                        if (sub > amax) amax = sub;
                    }
                }
            }
        }
        return amax;
    };

    // First, try parsed expression forms from _Alignas(...)
    for (const auto &e : specs.alignExprs) {
        if (!canon.empty()) canon += ";";
        if (!e) {
            canon += "<expr>";
            continue;
        }
        // try constant-eval
        auto v = ConstExprEvaluator::evalIntegerConstantExpr(e);
        if (v.has_value()) {
            if (!maxv.has_value() || *v > *maxv) maxv = v;
            // canonical text for integer literals
            if (e->kind == Expr::Kind::Integer) {
                auto il = std::dynamic_pointer_cast<IntegerLiteral>(e);
                canon += il ? il->raw : "<int>";
            } else {
                canon += "<const-expr>";
            }
            continue;
        }
        // if it's an AlignOfExpr with recorded typeText, attempt TU resolution
        if (e->kind == Expr::Kind::AlignOf) {
            auto ae = std::dynamic_pointer_cast<AlignOfExpr>(e);
            if (ae) {
                std::string norm;
                // normalize typeText to lower-case and single spaces
                bool lastSpace = false;
                for (char c : ae->typeText) {
                    if (isspace((unsigned char)c)) { if (!lastSpace) { norm.push_back(' '); lastSpace = true; } }
                    else { norm.push_back((char)tolower((unsigned char)c)); lastSpace = false; }
                }
                canon += std::string("_Alignof(") + ae->typeText + ")";
                // struct/union name
                if (norm.rfind("struct ", 0) == 0 || norm.rfind("union ", 0) == 0) {
                    std::string tag = norm.substr(norm.find(' ')+1);
                    if (tu_) {
                        for (const auto &ext : tu_->externals) {
                            if (!ext) continue;
                            if (auto decl = std::get_if<DeclarationPtr>(&ext->decl)) {
                                if (!*decl) continue;
                                for (const auto &ts : (*decl)->specifiers.typeSpecifiers) {
                                    if (ts.kind == DeclarationSpecifiers::TypeSpecifier::Kind::StructOrUnion && ts.su && ts.su->hasBody && ts.su->name && *ts.su->name == tag) {
                                        long long a = computeAlignForSU(ts.su);
                                        if (!maxv.has_value() || a > *maxv) maxv = a;
                                        goto next_expr;
                                    }
                                }
                            }
                        }
                    }
                } else {
                    // typedef resolution via TU
                    if (tu_) {
                        for (const auto &ext : tu_->externals) {
                            if (!ext) continue;
                            if (auto decl = std::get_if<DeclarationPtr>(&ext->decl)) {
                                if (!*decl) continue;
                                if ((*decl)->specifiers.hasStorage(StorageClass::Typedef) && (*decl)->declarator) {
                                    std::string dn = declaratorName((*decl)->declarator);
                                    if (dn == norm) {
                                        for (const auto &ts : (*decl)->specifiers.typeSpecifiers) {
                                            if (ts.kind == DeclarationSpecifiers::TypeSpecifier::Kind::StructOrUnion && ts.su && ts.su->hasBody) {
                                                long long a = computeAlignForSU(ts.su);
                                                if (!maxv.has_value() || a > *maxv) maxv = a;
                                                goto next_expr;
                                            }
                                            if (ts.kind == DeclarationSpecifiers::TypeSpecifier::Kind::Simple) {
                                                long long a = 4;
                                                for (auto st : ts.simple) {
                                                    using S = DeclarationSpecifiers::SimpleTypeSpecifier;
                                                    if (st == S::Char) a = std::max(a, 1LL);
                                                    else if (st == S::Short) a = std::max(a, 2LL);
                                                    else if (st == S::Int) a = std::max(a, 4LL);
                                                    else if (st == S::Long) a = std::max(a, 8LL);
                                                    else if (st == S::Float) a = std::max(a, 4LL);
                                                    else if (st == S::Double) a = std::max(a, 8LL);
                                                }
                                                if (!maxv.has_value() || a > *maxv) maxv = a;
                                                goto next_expr;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        // fallback canonical text for unknown expr
        canon += "<expr>";
        next_expr: ;
    }

    // Then process any legacy textual alignSpec strings
    if (!specs.alignSpec.empty()) {
        auto [val, c] = computeAlignFromSpecs(specs.alignSpec);
        if (!canon.empty() && !c.empty()) canon += ";";
        canon += c;
        if (val.has_value()) {
            if (!maxv.has_value() || *val > *maxv) maxv = val;
        }
    }
    return {maxv, canon};
}

// Structural equality for TypeNode trees. Compares Kind and recursively
// compares child nodes and relevant flags; does not consider source spans.
bool Semantic::typeNodesEqual(const std::shared_ptr<TypeNode> &a, const std::shared_ptr<TypeNode> &b) {
    if (a == b) return true;
    if (!a || !b) return false;
    if (a->kind != b->kind) return false;
    switch (a->kind) {
        case TypeNode::Kind::Builtin:
            if (a->simple != b->simple) return false;
            if (a->text != b->text) return false;
            return true;
        case TypeNode::Kind::Struct:
        case TypeNode::Kind::Union:
            // compare tag names if present
            if ((a->su && a->su->name.has_value()) != (b->su && b->su->name.has_value())) return false;
            if (a->su && b->su && a->su->name.has_value() && b->su->name.has_value()) return a->su->name == b->su->name;
            return true;
        case TypeNode::Kind::Enum:
            return a->text == b->text;
        case TypeNode::Kind::Pointer:
            if (a->ptrQual != b->ptrQual) return false;
            return typeNodesEqual(a->pointee, b->pointee);
        case TypeNode::Kind::Array:
            if (a->arrayIsStar != b->arrayIsStar) return false;
            if (a->arrayIsStatic != b->arrayIsStatic) return false;
            if (a->arrayQual != b->arrayQual) return false;
            // sizeExpr comparison: if both constant and integer literal, compare raw; otherwise ignore
            if (a->sizeExpr.has_value() != b->sizeExpr.has_value()) return false;
            if (a->sizeExpr.has_value() && b->sizeExpr.has_value()) {
                if (a->sizeExpr.value()->kind == Expr::Kind::Integer && b->sizeExpr.value()->kind == Expr::Kind::Integer) {
                    auto ia = std::dynamic_pointer_cast<IntegerLiteral>(a->sizeExpr.value());
                    auto ib = std::dynamic_pointer_cast<IntegerLiteral>(b->sizeExpr.value());
                    if (!ia || !ib) return false;
                    if (ia->raw != ib->raw) return false;
                }
            }
            return typeNodesEqual(a->element, b->element);
        case TypeNode::Kind::Function:
            if (a->hasParamTypeList != b->hasParamTypeList) return false;
            if (!typeNodesEqual(a->element, b->element)) return false;
            if (a->params.size() != b->params.size()) return false;
            for (size_t i = 0; i < a->params.size(); ++i) if (!typeNodesEqual(a->params[i], b->params[i])) return false;
            return true;
        case TypeNode::Kind::Qualified:
            return a->text == b->text;
    }
    return false;
}

// Redeclaration compatibility (C 6.2.7, 6.7.6.3p14): like typeNodesEqual, but a
// function type with no prototype (empty parameter list, `hasParamTypeList ==
// false`) is compatible with a prototyped function type that has the same return
// type — the composite type takes the prototype. Used only for redeclaration
// checks, not the stricter type-identity used by `_Generic`.
static bool redeclTypesCompatible(const std::shared_ptr<TypeNode> &a,
                                  const std::shared_ptr<TypeNode> &b) {
    if (a && b && a->kind == TypeNode::Kind::Function
        && b->kind == TypeNode::Kind::Function
        && (a->hasParamTypeList == false || b->hasParamTypeList == false)) {
        // Compatible iff the return types are compatible; the unprototyped side
        // imposes no parameter constraints.
        return Semantic::typeNodesEqual(a->element, b->element);
    }
    return Semantic::typeNodesEqual(a, b);
}

// Helper: determine whether an initializer is a constant (or composed of constants).
// Whether an expression is a valid constant for an object with static storage
// duration (6.6p7-9): an arithmetic constant expression, a null pointer
// constant, an address constant, or such combined with an integer constant.
// Broader than an integer constant expression — e.g. `(void (*)(int))0` (a
// null pointer constant cast to a function-pointer type) and `&obj` qualify.
bool Semantic::exprIsStaticInitConstant(const ExprPtr &e) const {
    if (!e) return false;
    if (e->kind == Expr::Kind::String) return true;
    if (e->kind == Expr::Kind::Float) return true;     // floating constant
    if (ConstExprEvaluator::isIntegerConstantExpr(e)) return true;
    // A bare identifier naming an array or function decays to an address
    // constant (`static int *p = arr;`, `static fn_t f = func;`); a scalar
    // object's value is not a constant.
    if (e->kind == Expr::Kind::Ident) {
        auto id = std::static_pointer_cast<IdentifierExpr>(e);
        return addressConstantNames_.count(id->name) != 0;
    }
    if (e->kind == Expr::Kind::Cast) {
        auto ce = std::static_pointer_cast<CastExpr>(e);
        return exprIsStaticInitConstant(ce->expr); // a cast of a constant is constant
    }
    if (e->kind == Expr::Kind::Unary) {
        auto ue = std::static_pointer_cast<UnaryExpr>(e);
        if (ue->op == "&") return true;            // address constant: &object
        if (ue->op == "+" || ue->op == "-" || ue->op == "~" || ue->op == "!")
            return exprIsStaticInitConstant(ue->rhs);
    }
    if (e->kind == Expr::Kind::Binary) {
        auto be = std::static_pointer_cast<BinaryExpr>(e);
        if (be->op != ",")
            return exprIsStaticInitConstant(be->lhs) && exprIsStaticInitConstant(be->rhs);
    }
    if (e->kind == Expr::Kind::Ternary) {
        auto te = std::static_pointer_cast<TernaryExpr>(e);
        return exprIsStaticInitConstant(te->cond)
            && exprIsStaticInitConstant(te->thenExpr)
            && exprIsStaticInitConstant(te->elseExpr);
    }
    if (e->kind == Expr::Kind::Sizeof || e->kind == Expr::Kind::AlignOf) return true;
    // A file-scope compound literal has static storage duration (6.5.2.5p5): it
    // is a valid static initializer when its own initializer list is constant
    // (used by value, or via its address-constant array decay).
    if (e->kind == Expr::Kind::CompoundLiteral) {
        auto cl = std::static_pointer_cast<CompoundLiteral>(e);
        std::vector<wvmcc::Diagnostic> tmp;
        return initializerIsConstant(cl->init, tmp);
    }
    return false;
}

bool Semantic::initializerIsConstant(const InitializerPtr &init, std::vector<wvmcc::Diagnostic> &diagnostics) const {
    if (!init) return false;
    if (init->kind == Initializer::Kind::Expr) {
        if (!init->expr) return false;
        return exprIsStaticInitConstant(init->expr);
    }
    // list: all clauses' inits must be constant
    for (const auto &cl : init->clauses) {
        if (cl.init) {
            if (!initializerIsConstant(cl.init, diagnostics)) return false;
        } else {
            return false;
        }
        // if designator index present, ensure it's integer-constant
        for (const auto &d : cl.designators) {
            if (d.kind == Designator::Kind::Index) {
                if (!d.index || !ConstExprEvaluator::isIntegerConstantExpr(*d.index)) {
                    wvmcc::Diagnostic diag;
                    diag.severity = wvmcc::Diagnostic::Severity::Error;
                    diag.message = "designator index must be an integer constant expression";
                    if (d.index) diag.span = d.index.value()->span;
                    diagnostics.push_back(std::move(diag));
                    return false;
                }
            }
        }
    }
    return true;
}
// Check designator indexes across an initializer (regardless of storage class)
static bool checkDesignatorIndexes(const InitializerPtr &init, std::vector<wvmcc::Diagnostic> &diagnostics) {
    if (!init) return true;
    if (init->kind == Initializer::Kind::Expr) return true;
    for (const auto &cl : init->clauses) {
        for (const auto &d : cl.designators) {
            if (d.kind == Designator::Kind::Index) {
                if (!d.index || !ConstExprEvaluator::isIntegerConstantExpr(*d.index)) {
                    wvmcc::Diagnostic diag;
                    diag.severity = wvmcc::Diagnostic::Severity::Error;
                    diag.message = "designator index must be an integer constant expression";
                    if (d.index) diag.span = d.index.value()->span;
                    diagnostics.push_back(std::move(diag));
                    return false;
                }
            }
        }
        if (cl.init) {
            if (!checkDesignatorIndexes(cl.init, diagnostics)) return false;
        }
    }
    return true;
}
// Find the first array declarator (outermost-first) that has no size expression
static DeclaratorPtr findFirstArrayDeclaratorWithoutSize(const DeclaratorPtr &d) {
    if (!d) return nullptr;
    DeclaratorPtr cur = d;
    while (cur) {
        if (cur->kind == Declarator::Kind::Array) {
            if (!cur->array.size.has_value()) return cur;
        }
        if (cur->inner.has_value()) cur = cur->inner.value(); else break;
    }
    return nullptr;
}

// Create an integer literal expression node for a given numeric value
static ExprPtr makeIntegerLiteral(long long v) {
    auto il = make_ast<IntegerLiteral>();
    il->kind = Expr::Kind::Integer;
    il->value = v;
    il->raw = std::to_string(v);
    return il;
}

// C 6.7.6.2p1: the size expression of an array declarator, when it is an
// integer constant expression, shall have a value greater than zero. Walk all
// array layers of a declarator and diagnose any constant non-positive size.
// Non-constant sizes (VLAs) are not checked here.
static void checkArraySizes(const DeclaratorPtr &decl, std::vector<wvmcc::Diagnostic> &diagnostics) {
    DeclaratorPtr cur = decl;
    while (cur) {
        if (cur->kind == Declarator::Kind::Array && cur->array.size.has_value() && cur->array.size.value()) {
            const auto &sz = cur->array.size.value();
            if (ConstExprEvaluator::isIntegerConstantExpr(sz)) {
                auto v = ConstExprEvaluator::evalIntegerConstantExpr(sz);
                if (v.has_value() && *v <= 0) {
                    wvmcc::Diagnostic diag; diag.severity = wvmcc::Diagnostic::Severity::Error;
                    diag.message = (*v == 0)
                        ? "array size must be greater than zero"
                        : "array size is negative";
                    diag.span = sz->span;
                    diagnostics.push_back(std::move(diag));
                }
            }
        }
        if (cur->inner.has_value()) cur = cur->inner.value(); else break;
    }
}

// Recursive validation of an initializer against a TypeNode. This implements
// a conservative, structural traversal of aggregate initializers to:
// - validate nested initializer forms (list vs expr) against subobject types
// - complete array sizes for arrays of unknown size when initializer-list
//   without designators is provided
// - report simple excess-element and unknown-member designator errors
static void validateInitializerAgainstType(const std::shared_ptr<TypeNode> &type, const InitializerPtr &init, std::vector<wvmcc::Diagnostic> &diagnostics, bool isStaticStorage, const Semantic &sem) {
    if (!type || !init) return;
    // Scalars: expect an expression (or a single-element braced list)
    if (type->kind == TypeNode::Kind::Builtin || type->kind == TypeNode::Kind::Enum || type->kind == TypeNode::Kind::Qualified) {
        if (init->kind == Initializer::Kind::List) {
            size_t n = init->clauses.size();
            if (n == 0) return; // empty braced initializer -> OK
            if (n > 1) {
                Diagnostic diag; diag.severity = Diagnostic::Severity::Error;
                diag.message = "too many initializers for scalar";
                diag.span = init->span;
                diagnostics.push_back(std::move(diag));
            }
            // if exactly one, treat its nested init as an assignment-expression
            if (n == 1 && init->clauses[0].init) {
                if (init->clauses[0].init->kind == Initializer::Kind::Expr) return;
                // nested list for scalar -> error
                Diagnostic diag; diag.severity = Diagnostic::Severity::Error;
                diag.message = "invalid initializer for scalar";
                diag.span = init->clauses[0].init ? init->clauses[0].init->span : init->span;
                diagnostics.push_back(std::move(diag));
            }
        }
        return;
    }

    // Arrays
    if (type->kind == TypeNode::Kind::Array) {
        auto elem = type->element;
        if (init->kind == Initializer::Kind::Expr) {
            // string literal special-case handled elsewhere; otherwise valid
            return;
        }
        // list form: support designated initializers and sequential mapping
        size_t nclauses = init->clauses.size();
        // detect if any designators exist at top level
        bool anyDesignators = false;
        for (const auto &cl : init->clauses) if (!cl.designators.empty()) { anyDesignators = true; break; }

        // Complete an unknown-size array from its initializer (6.7.9p22): the
        // size is the largest index initialized, plus one — counting both
        // designated indices and the sequential positions between/after them.
        // A designator `[k]` repositions the cursor to k; the next undesignated
        // clause goes to k+1. Computing the full extent up front (rather than
        // letting the first designator fix the size) is what lets a later, larger
        // index like `{[0]=1, [7]=8, [3]=4}` size the array to 8 instead of being
        // rejected as out of range.
        if (!type->sizeExpr.has_value()) {
            long long cursor = 0;  // index the next clause initializes
            long long maxLen = 0;  // highest (index + 1) reached
            for (const auto &cl : init->clauses) {
                if (!cl.designators.empty()
                    && cl.designators.front().kind == Designator::Kind::Index
                    && cl.designators.front().index) {
                    auto vi = ConstExprEvaluator::evalIntegerConstantExpr(cl.designators.front().index.value());
                    if (vi.has_value()) cursor = *vi;
                }
                cursor += 1;
                if (cursor > maxLen) maxLen = cursor;
            }
            type->sizeExpr = makeIntegerLiteral(maxLen);
        }

        // Helper to map a clause with designators to a subobject type
        auto mapDesignatorsToType = [&](const std::vector<Designator> &designators)->std::shared_ptr<TypeNode> {
            std::shared_ptr<TypeNode> cur = type;
            for (const auto &d : designators) {
                if (!cur) return nullptr;
                if (d.kind == Designator::Kind::Index) {
                    if (cur->kind != TypeNode::Kind::Array) {
                        Diagnostic diag; diag.severity = Diagnostic::Severity::Error;
                        diag.message = "index designator applied to non-array type";
                        diag.span = d.index ? d.index.value()->span : init->span;
                        diagnostics.push_back(std::move(diag));
                        return nullptr;
                    }
                    if (!d.index) {
                        Diagnostic diag; diag.severity = Diagnostic::Severity::Error;
                        diag.message = "missing index in designator";
                        diag.span = init->span;
                        diagnostics.push_back(std::move(diag));
                        return nullptr;
                    }
                    auto vi = ConstExprEvaluator::evalIntegerConstantExpr(d.index.value());
                    if (!vi.has_value()) {
                        Diagnostic diag; diag.severity = Diagnostic::Severity::Error;
                        diag.message = "designator index must be an integer constant expression";
                        diag.span = d.index.value()->span;
                        diagnostics.push_back(std::move(diag));
                        return nullptr;
                    }
                    long long idx = *vi;
                    // Complete array size if unknown
                    if (!cur->sizeExpr.has_value()) {
                        cur->sizeExpr = makeIntegerLiteral(idx + 1);
                    } else {
                        auto vsz = ConstExprEvaluator::evalIntegerConstantExpr(cur->sizeExpr.value());
                        if (vsz.has_value()) {
                            if (idx >= *vsz) {
                                Diagnostic diag; diag.severity = Diagnostic::Severity::Error;
                                diag.message = "designator index out of range";
                                diag.span = d.index.value()->span;
                                diagnostics.push_back(std::move(diag));
                                // continue mapping to element type nonetheless
                            }
                        }
                    }
                    cur = cur->element;
                } else if (d.kind == Designator::Kind::Member) {
                    if (cur->kind != TypeNode::Kind::Struct && cur->kind != TypeNode::Kind::Union) {
                        Diagnostic diag; diag.severity = Diagnostic::Severity::Error;
                        diag.message = "member designator applied to non-struct/union type";
                        diag.span = init->span;
                        diagnostics.push_back(std::move(diag));
                        return nullptr;
                    }
                    bool found = false;
                    if (cur->su && cur->su->hasBody) {
                        for (const auto &m : cur->su->members) {
                            for (const auto &sd : m.declarators) {
                                if (sd.declarator) {
                                    if (declaratorName(sd.declarator) == d.member) {
                                        DeclarationSpecifiers ms = m.specifiers;
                                        bool vm = false;
                                        cur = sem.buildTypeFromDeclaration(ms, sd.declarator, false, &vm);
                                        found = true;
                                        break;
                                    }
                                }
                            }
                            if (found) break;
                        }
                    }
                    if (!found) {
                        Diagnostic diag; diag.severity = Diagnostic::Severity::Error;
                        diag.message = "designator refers to unknown member '" + d.member + "'";
                        diag.span = init->span;
                        diagnostics.push_back(std::move(diag));
                        return nullptr;
                    }
                }
            }
            return cur;
        };

        // If any designators present, map each clause individually
        if (anyDesignators) {
            for (const auto &cl : init->clauses) {
                if (!cl.designators.empty()) {
                    auto target = mapDesignatorsToType(cl.designators);
                    if (target && cl.init) validateInitializerAgainstType(target, cl.init, diagnostics, isStaticStorage, sem);
                } else {
                    // no designators: fall back to sequential mapping
                    // sequential mapping handled below after building member/element sequence
                }
            }
            // also handle non-designator clauses sequentially below by falling through
        }

        // Recurse per element for non-designated clauses (sequential mapping)
        for (size_t i = 0; i < nclauses; ++i) {
            const auto &cl = init->clauses[i];
            if (!cl.designators.empty()) continue; // already handled
            if (cl.init) validateInitializerAgainstType(elem, cl.init, diagnostics, isStaticStorage, sem);
        }
        return;
    }

    // Structs: map initializer clauses to members in order (simple support)
    if (type->kind == TypeNode::Kind::Struct && type->su && type->su->hasBody) {
        // Build list of named members' type nodes
        std::vector<std::shared_ptr<TypeNode>> memberTypes;
        for (const auto &m : type->su->members) {
            for (const auto &sd : m.declarators) {
                if (sd.declarator) {
                    // Build the member's declaration specifiers
                    DeclarationSpecifiers ms = m.specifiers;
                    // Use Semantic::buildTypeFromDeclaration to construct member type
                    bool vm = false;
                    auto mtn = sem.buildTypeFromDeclaration(ms, sd.declarator, false, &vm);
                    memberTypes.push_back(mtn);
                }
            }
            // anonymous members (no declarators) are treated as nested anonymous structs/unions
            for (const auto &ts : m.specifiers.typeSpecifiers) {
                if (ts.kind == DeclarationSpecifiers::TypeSpecifier::Kind::StructOrUnion && ts.su && ts.su->hasBody && m.declarators.empty()) {
                    // anonymous nested specifier: expose its members as flattened sequence
                    for (const auto &nm : ts.su->members) {
                        for (const auto &nsd : nm.declarators) {
                            if (nsd.declarator) {
                                DeclarationSpecifiers nms = nm.specifiers;
                                bool nvm = false;
                                auto ntn = sem.buildTypeFromDeclaration(nms, nsd.declarator, false, &nvm);
                                memberTypes.push_back(ntn);
                            }
                        }
                    }
                }
            }
        }
        // If initializer is an expr, it's invalid except when single-member aggregate? Conservative: error
        if (init->kind == Initializer::Kind::Expr) {
            Diagnostic diag; diag.severity = Diagnostic::Severity::Error;
            diag.message = "invalid initializer for struct/union";
            diag.span = init->span;
            diagnostics.push_back(std::move(diag));
            return;
        }
        // Map clauses in order to members
        size_t mcount = memberTypes.size();
        size_t ccount = init->clauses.size();
        size_t idx = 0;
        for (size_t i = 0; i < ccount; ++i) {
            const auto &cl = init->clauses[i];
            if (!cl.designators.empty()) {
                // Basic designated member check handled elsewhere; conservatively skip mapping
                continue;
            }
            if (idx >= mcount) {
                Diagnostic diag; diag.severity = Diagnostic::Severity::Error;
                diag.message = "excess elements in initializer";
                diag.span = init->span;
                diagnostics.push_back(std::move(diag));
                break;
            }
            if (cl.init) validateInitializerAgainstType(memberTypes[idx], cl.init, diagnostics, isStaticStorage, sem);
            idx++;
        }
        return;
    }

    // Unions: simple check — accept a single initializer for the first member or a designated member
    if (type->kind == TypeNode::Kind::Union && type->su && type->su->hasBody) {
        if (init->kind == Initializer::Kind::Expr) return;
        // list form: if more than one clause without designators, error
        size_t n = init->clauses.size();
        if (n > 1) {
            Diagnostic diag; diag.severity = Diagnostic::Severity::Error;
            diag.message = "too many initializers for union";
            diag.span = init->span;
            diagnostics.push_back(std::move(diag));
            return;
        }
        if (n == 1 && init->clauses[0].init) {
            // find first member type
            for (const auto &m : type->su->members) {
                for (const auto &sd : m.declarators) {
                    if (sd.declarator) {
                        DeclarationSpecifiers ms = m.specifiers;
                        bool vm = false;
                        auto mtn = sem.buildTypeFromDeclaration(ms, sd.declarator, false, &vm);
                        validateInitializerAgainstType(mtn, init->clauses[0].init, diagnostics, isStaticStorage, sem);
                        return;
                    }
                }
            }
        }
        return;
    }
}

// Create a compact signature string for a declaration's type/specifiers/declarator
static std::string signatureForDeclaration(const DeclarationPtr &d) {
    if (!d) return std::string();
    std::string s;

    // include top-level type qualifiers
    if (d->specifiers.typeQualFlags != TypeQualifier::None) {
        s += "quals:" + std::to_string(static_cast<int>(d->specifiers.typeQualFlags)) + ";";
    }

    // specifiers (including `_Atomic(inner)` handling)
    for (const auto &ts : d->specifiers.typeSpecifiers) {
        switch (ts.kind) {
            case DeclarationSpecifiers::TypeSpecifier::Kind::Simple:
                s += "simple:";
                for (auto st : ts.simple) s += std::to_string(static_cast<int>(st)) + ",";
                s += ";";
                break;
            case DeclarationSpecifiers::TypeSpecifier::Kind::StructOrUnion:
                s += (ts.su->kind == StructOrUnionSpecifier::Kind::Struct) ? "struct:" : "union:";
                if (ts.su->name) s += *ts.su->name; else s += "<anon>";
                s += ts.su->hasBody ? ":body;" : ":fwd;";
                break;
            case DeclarationSpecifiers::TypeSpecifier::Kind::Enum:
                s += "enum:";
                if (ts.en->name) s += *ts.en->name; else s += "<anon>";
                s += ts.en->hasBody ? ":body;" : ":fwd;";
                break;
            case DeclarationSpecifiers::TypeSpecifier::Kind::Atomic:
                s += "atomic:";
                if (ts.atomicInner) {
                    if (ts.atomicInner->typeQualFlags != TypeQualifier::None) s += "innerquals:" + std::to_string(static_cast<int>(ts.atomicInner->typeQualFlags)) + ";";
                    for (const auto &its : ts.atomicInner->typeSpecifiers) {
                        switch (its.kind) {
                            case DeclarationSpecifiers::TypeSpecifier::Kind::Simple:
                                s += "inner_simple:";
                                for (auto st : its.simple) s += std::to_string(static_cast<int>(st)) + ",";
                                s += ";";
                                break;
                            case DeclarationSpecifiers::TypeSpecifier::Kind::StructOrUnion:
                                s += (its.su->kind == StructOrUnionSpecifier::Kind::Struct) ? "inner_struct:" : "inner_union:";
                                if (its.su->name) s += *its.su->name; else s += "<anon>";
                                s += its.su->hasBody ? ":body;" : ":fwd;";
                                break;
                            case DeclarationSpecifiers::TypeSpecifier::Kind::Enum:
                                s += "inner_enum:";
                                if (its.en->name) s += *its.en->name; else s += "<anon>";
                                s += its.en->hasBody ? ":body;" : ":fwd;";
                                break;
                            case DeclarationSpecifiers::TypeSpecifier::Kind::TypedefName:
                                s += "inner_typedef:" + its.text + ";";
                                break;
                            case DeclarationSpecifiers::TypeSpecifier::Kind::Other:
                                s += "inner_other:" + its.text + ";";
                                break;
                            default:
                                break;
                        }
                    }
                }
                break;
            case DeclarationSpecifiers::TypeSpecifier::Kind::TypedefName:
                s += "typedef:" + ts.text + ";";
                break;
            case DeclarationSpecifiers::TypeSpecifier::Kind::Other:
                s += "other:" + ts.text + ";";
                break;
        }
    }

    // declarator kind basics
    if (d->declarator) {
        s += "declkind:" + std::to_string(static_cast<int>(d->declarator->kind));
        if (d->declarator->kind == Declarator::Kind::Function) {
            s += ":params=" + std::to_string(d->declarator->function.params.size());
        }
    }

    // include pointer/array qualifiers in nested declarators
    auto cur = d->declarator;
    while (cur) {
        if (cur->kind == Declarator::Kind::Pointer) {
            if (cur->ptrQual != TypeQualifier::None) s += "ptrquals:" + std::to_string(static_cast<int>(cur->ptrQual)) + ";";
        } else if (cur->kind == Declarator::Kind::Array) {
            if (cur->array.qual != TypeQualifier::None) s += "arrquals:" + std::to_string(static_cast<int>(cur->array.qual)) + ";";
        }
        if (cur->inner.has_value()) cur = cur->inner.value(); else break;
    }

    return s;
}

// Strip qualifier-related substrings from a signature so structural comparison
// can ignore qualifiers. Removes 'quals:', 'innerquals:', 'ptrquals:', 'arrquals:'.
static std::string stripQualParts(const std::string &sig) {
    std::string out = sig;
    const std::vector<std::string> parts = {"quals:", "innerquals:", "ptrquals:", "arrquals:"};
    for (const auto &p : parts) {
        size_t pos = 0;
        while ((pos = out.find(p, pos)) != std::string::npos) {
            // find end of numeric value (terminated by ';')
            size_t semi = out.find(';', pos);
            if (semi == std::string::npos) { out.erase(pos); break; }
            out.erase(pos, semi - pos + 1);
        }
    }
    return out;
}
// Extract the identifier name from possibly-nested declarators
static std::string declaratorName(const DeclaratorPtr &d) {
    if (!d) return std::string();
    DeclaratorPtr cur = d;
    while (cur) {
        if (cur->kind == Declarator::Kind::Identifier) return cur->id.name;
        if (cur->inner.has_value()) cur = cur->inner.value();
        else break;
    }
    return std::string();
}

// Determine whether a declarator denotes a function (possibly nested)
static bool isFunctionDeclarator(const DeclaratorPtr &d) {
    if (!d) return false;
    DeclaratorPtr cur = d;
    while (cur) {
        if (cur->kind == Declarator::Kind::Function) return true;
        if (cur->inner.has_value()) cur = cur->inner.value();
        else break;
    }
    return false;
}

// 6.7.2.1p3: a struct/union shall not contain a member of incomplete type — in
// particular not a (non-pointer) instance of itself, whose type is incomplete
// until the closing brace. Scans the struct/union bodies in `specs` and reports
// each self-containing member. Runs independent of any declarator so a bare
// `struct node { struct node next; };` is diagnosed.
static void checkStructSelfContainment(const DeclarationSpecifiers &specs,
                                       const wvmcc::SourceSpan &span,
                                       std::vector<wvmcc::Diagnostic> &diagnostics) {
    for (const auto &ts : specs.typeSpecifiers) {
        if (ts.kind != DeclarationSpecifiers::TypeSpecifier::Kind::StructOrUnion)
            continue;
        if (!ts.su || !ts.su->hasBody || !ts.su->name.has_value()) continue;
        const std::string &tag = *ts.su->name;
        for (const auto &mem : ts.su->members) {
            // A member referencing the enclosing tag is a self-reference. (The
            // parser reuses the same specifier object for the member, so its
            // `hasBody` flag is not a reliable "incomplete" signal — match by
            // tag name and let the pointer check below allow `struct node*`.)
            bool selfRef = false;
            for (const auto &mts : mem.specifiers.typeSpecifiers) {
                if (mts.kind == DeclarationSpecifiers::TypeSpecifier::Kind::StructOrUnion
                    && mts.su && mts.su->name.has_value() && *mts.su->name == tag) {
                    selfRef = true; break;
                }
            }
            if (!selfRef) continue;
            for (const auto &sd : mem.declarators) {
                if (!sd.declarator) continue;
                // A pointer anywhere in the declarator (`struct node *next`,
                // `struct node *a[3]`) puts the struct behind an indirection and
                // is fine; a direct object or array of the (still-incomplete)
                // struct is not.
                bool hasPointer = false;
                for (DeclaratorPtr cur = sd.declarator; cur;
                     cur = cur->inner.has_value() ? *cur->inner : nullptr) {
                    if (cur->kind == Declarator::Kind::Pointer) { hasPointer = true; break; }
                }
                if (!hasPointer) {
                    Diagnostic diag;
                    diag.severity = Diagnostic::Severity::Error;
                    diag.message = "struct/union member has incomplete type (contains an instance of itself)";
                    diag.span = span;
                    diagnostics.push_back(std::move(diag));
                }
            }
        }
    }
}

// 6.7.6.2p1 / 6.7.6.3p1: a function declarator shall not specify a return type
// that is a function or array type, and an array element type shall not be a
// function type. Detect the direct Function/Array adjacency in a declarator
// chain (a pointer in between — `int (*f(void))[3]` — breaks it and is legal).
// Returns an error message or empty string.
static std::string illegalFuncArrayCombo(const DeclaratorPtr &d) {
    DeclaratorPtr cur = d;
    while (cur && cur->inner.has_value() && *cur->inner) {
        auto inner = *cur->inner;
        if (cur->kind == Declarator::Kind::Function) {
            if (inner->kind == Declarator::Kind::Array)
                return "function return type may not be an array type";
            if (inner->kind == Declarator::Kind::Function)
                return "function return type may not be a function type";
        }
        if (cur->kind == Declarator::Kind::Array
            && inner->kind == Declarator::Kind::Function)
            return "array element type may not be a function type";
        cur = inner;
    }
    return "";
}

// 6.7.2p2: the multiset of simple type-specifiers in a declaration shall be one
// of a fixed list of valid combinations. Returns a non-empty error message for
// a positively-invalid multiset (e.g. `signed unsigned`, `char int`,
// `float int`), or an empty string when the combination is valid or contains a
// struct/union/enum/typedef/atomic specifier (which this check ignores).
static std::string invalidSimpleTypeMultiset(const DeclarationSpecifiers &specs) {
    using S = DeclarationSpecifiers::SimpleTypeSpecifier;
    int nVoid=0,nChar=0,nBool=0,nFloat=0,nDouble=0,nInt=0,nShort=0,nLong=0,
        nSigned=0,nUnsigned=0;
    bool sawSimple=false;
    for (const auto &ts : specs.typeSpecifiers) {
        if (ts.kind != DeclarationSpecifiers::TypeSpecifier::Kind::Simple) {
            // A non-simple specifier (struct/union/enum/typedef/atomic): leave
            // multiset validation to the type builder — don't risk a false flag.
            return "";
        }
        for (auto s : ts.simple) {
            sawSimple = true;
            switch (s) {
                case S::Void: nVoid++; break;
                case S::Char: nChar++; break;
                case S::Bool: nBool++; break;
                case S::Float: nFloat++; break;
                case S::Double: nDouble++; break;
                case S::Int: nInt++; break;
                case S::Short: nShort++; break;
                case S::Long: nLong++; break;
                case S::Signed: nSigned++; break;
                case S::Unsigned: nUnsigned++; break;
                default: return ""; // _Complex/_Imaginary — out of scope, don't flag
            }
        }
    }
    if (!sawSimple) return "";
    const char *dup = "duplicate type specifier in declaration";
    if (nVoid>1||nChar>1||nBool>1||nFloat>1||nDouble>1||nInt>1||nShort>1) return dup;
    if (nSigned>1||nUnsigned>1) return dup;
    if (nLong>2) return "too many 'long' specifiers";
    if (nSigned>0 && nUnsigned>0) return "both 'signed' and 'unsigned' in declaration";
    bool hasSign = (nSigned>0 || nUnsigned>0);
    if (nVoid>0 && (nChar||nBool||nFloat||nDouble||nInt||nShort||nLong||hasSign))
        return "'void' combined with another type specifier";
    if (nBool>0 && (nChar||nFloat||nDouble||nInt||nShort||nLong||hasSign))
        return "'_Bool' combined with another type specifier";
    if (nChar>0 && (nFloat||nDouble||nInt||nShort||nLong))
        return "'char' combined with an incompatible type specifier";
    if (nFloat>0 && (nDouble||nInt||nShort||nLong||hasSign||nChar))
        return "'float' combined with another type specifier";
    if (nDouble>0 && (nInt||nShort||hasSign||nChar)) // `long double` permitted
        return "'double' combined with an incompatible type specifier";
    if (nDouble>0 && nLong>1) return "too many 'long' specifiers";
    if (nShort>0 && nLong>0) return "both 'short' and 'long' in declaration";
    return "";
}

// Check whether a struct/union specifier contains at least one named member,
// directly or via anonymous nested structs/unions.
static bool structOrUnionHasNamedMember(const std::shared_ptr<StructOrUnionSpecifier> &su) {
    if (!su) return false;
    for (const auto &m : su->members) {
        // if any declarator in this member declares an identifier, we have a named member
        for (const auto &sd : m.declarators) {
            if (sd.declarator) {
                if (!declaratorName(sd.declarator).empty()) return true;
            }
        }
        // no declarators: could be an anonymous struct/union specifier in specifiers
        for (const auto &ts : m.specifiers.typeSpecifiers) {
            if (ts.kind == DeclarationSpecifiers::TypeSpecifier::Kind::StructOrUnion) {
                if (ts.su && ts.su->hasBody) {
                    if (structOrUnionHasNamedMember(ts.su)) return true;
                }
            }
        }
    }
    return false;
}

// Build a simple TypeNode representation from declaration specifiers and a
// possibly-nested declarator. This produces a readable `repr` and sets the
// `kind` to a best-effort value. It also detects variably-modified types
// (VLAs) when array sizes are non-constant.
std::shared_ptr<TypeNode> Semantic::buildTypeFromDeclaration(const DeclarationSpecifiers &specs, const DeclaratorPtr &decl, bool inParamPrototype, bool *outVariablyModified) const {
    auto tn = std::make_shared<TypeNode>();
    bool variablyModified = false;

    // Base type from specs
    if (!specs.typeSpecifiers.empty()) {
        const auto &ts = specs.typeSpecifiers.front();
        switch (ts.kind) {
            case DeclarationSpecifiers::TypeSpecifier::Kind::Simple:
                tn->kind = TypeNode::Kind::Builtin;
                tn->simple = ts.simple;
                break;
            case DeclarationSpecifiers::TypeSpecifier::Kind::StructOrUnion:
                tn->kind = (ts.su && ts.su->kind == StructOrUnionSpecifier::Kind::Struct) ? TypeNode::Kind::Struct : TypeNode::Kind::Union;
                tn->su = ts.su;
                break;
            case DeclarationSpecifiers::TypeSpecifier::Kind::Enum:
                tn->kind = TypeNode::Kind::Enum;
                tn->text = ts.en && ts.en->name.has_value() ? *ts.en->name : std::string();
                break;
            case DeclarationSpecifiers::TypeSpecifier::Kind::TypedefName:
                tn->kind = TypeNode::Kind::Builtin;
                tn->text = ts.text;
                break;
            case DeclarationSpecifiers::TypeSpecifier::Kind::Atomic:
                tn->kind = TypeNode::Kind::Qualified;
                tn->text = "_Atomic";
                if (ts.atomicInner) {
                    tn->pointee = buildTypeFromDeclaration(*ts.atomicInner, nullptr, inParamPrototype, outVariablyModified);
                }
                break;
            case DeclarationSpecifiers::TypeSpecifier::Kind::Other:
                tn->kind = TypeNode::Kind::Builtin;
                tn->text = ts.text;
                break;
        }
    } else {
        tn->kind = TypeNode::Kind::Builtin;
        // default int: represent as empty simple (fallback)
    }

    // Collect declarator layers (outer->inner) and apply inner-first
    std::vector<DeclaratorPtr> layers;
    DeclaratorPtr cur = decl;
    while (cur) { layers.push_back(cur); if (cur->inner.has_value()) cur = cur->inner.value(); else break; }
    // Array subscripts nest rightmost-outermost in the declarator chain, but C
    // requires the leftmost subscript to be the outermost dimension. Reverse
    // each maximal run of adjacent Array layers so the inner-first wrapping
    // below yields the correct dimension order (int[2][3] -> array[2] of
    // array[3] of int). Pointer/function layers break the run, so the spiral
    // rule (int *a[3] vs int (*a)[3]) is preserved.
    for (size_t s = 0; s < layers.size(); ) {
        if (layers[s] && layers[s]->kind == Declarator::Kind::Array) {
            size_t e = s;
            while (e < layers.size() && layers[e] && layers[e]->kind == Declarator::Kind::Array) ++e;
            std::reverse(layers.begin() + s, layers.begin() + e);
            s = e;
        } else ++s;
    }
    // Start from baseType and wrap
    std::shared_ptr<TypeNode> curType = tn;
    for (int i = static_cast<int>(layers.size()) - 1; i >= 0; --i) {
        auto layer = layers[i];
        if (!layer) continue;
        if (layer->kind == Declarator::Kind::Pointer) {
            auto p = std::make_shared<TypeNode>();
            p->kind = TypeNode::Kind::Pointer;
            p->pointee = curType;
            p->ptrQual = layer->ptrQual;
            curType = p;
        } else if (layer->kind == Declarator::Kind::Array) {
            auto a = std::make_shared<TypeNode>();
            a->kind = TypeNode::Kind::Array;
            a->element = curType;
            a->arrayIsStar = layer->array.isStar;
            a->arrayIsStatic = layer->array.isStatic;
            a->arrayQual = layer->array.qual;
            if (layer->array.size.has_value()) a->sizeExpr = layer->array.size.value();
            if (a->arrayIsStar || (a->sizeExpr.has_value() && !ConstExprEvaluator::isIntegerConstantExpr(a->sizeExpr.value()))) {
                if (!inParamPrototype) variablyModified = true;
            }
            curType = a;
        } else if (layer->kind == Declarator::Kind::Function) {
            auto f = std::make_shared<TypeNode>();
            f->kind = TypeNode::Kind::Function;
            // return type is curType
            // represent return type as element (reuse element field)
            f->element = curType;
            f->hasParamTypeList = layer->function.hasParamTypeList;
            f->isVariadic = layer->function.isVariadic;
            // A lone `(void)` parameter list denotes zero parameters — skip it,
            // so the Function type carries no phantom param (which would
            // otherwise surface as a spurious i32 in call_indirect / sizeof).
            const auto& fps = layer->function.params;
            bool voidParams = fps.size() == 1 && !fps[0].declarator && [&]{
                for (const auto& ts : fps[0].specifiers.typeSpecifiers)
                    if (ts.kind == DeclarationSpecifiers::TypeSpecifier::Kind::Simple
                        && ts.simple.size() == 1
                        && ts.simple[0] == DeclarationSpecifiers::SimpleTypeSpecifier::Void)
                        return true;
                return false;
            }();
            if (!voidParams) for (const auto &p : fps) {
                // best-effort: build param type nodes (limited info)
                std::shared_ptr<TypeNode> ptn = nullptr;
                if (p.declarator) ptn = buildTypeFromDeclaration(DeclarationSpecifiers(), p.declarator, true, nullptr);
                else if (p.typeSpec.has_value()) { ptn = std::make_shared<TypeNode>(); ptn->kind = TypeNode::Kind::Builtin; ptn->text = *p.typeSpec; }
                if (!ptn) ptn = std::make_shared<TypeNode>();
                f->params.push_back(ptn);
            }
            curType = f;
        }
    }

    if (outVariablyModified) *outVariablyModified = variablyModified;
    return curType;
}

// Resolve typedef-names by scanning the translation unit and produce a
// canonical type representation string. This recursively expands typedefs
// to their underlying type representations when possible.
std::shared_ptr<TypeNode> Semantic::canonicalTypeRepr(const DeclarationSpecifiers &specs, const DeclaratorPtr &decl) const {
    // Helper: resolve a typedef name to its canonical repr by finding its
    // typedef declaration in the TU. Use `visited` to avoid infinite recursion.
    std::function<std::shared_ptr<TypeNode>(const std::string&, std::unordered_set<std::string>&)> resolveTypedef;
    resolveTypedef = [&](const std::string &td, std::unordered_set<std::string> &visited)->std::shared_ptr<TypeNode> {
        if (visited.count(td)) return nullptr; // cycle -> return null
        visited.insert(td);
        if (!tu_) return nullptr;
        for (const auto &ext : tu_->externals) {
            if (!ext) continue;
            if (auto pdecl = std::get_if<DeclarationPtr>(&ext->decl)) {
                if (!*pdecl) continue;
                if ((*pdecl)->specifiers.hasStorage(StorageClass::Typedef) && (*pdecl)->declarator) {
                    std::string dn = declaratorName((*pdecl)->declarator);
                    if (dn == td) {
                        // recursively canonicalize the typedef's underlying type
                        return canonicalTypeRepr((*pdecl)->specifiers, (*pdecl)->declarator);
                    }
                }
            }
        }
        return nullptr;
    };

    // Helper: apply declarator layers (pointer/array/function) onto a base repr
    auto applyDeclaratorLayers = [&](const std::shared_ptr<TypeNode> &base, const DeclaratorPtr &d, bool inParamPrototype, bool *outVM)->std::shared_ptr<TypeNode> {
        std::shared_ptr<TypeNode> cur = base ? base : std::make_shared<TypeNode>();
        bool variablyModified = false;
        std::vector<DeclaratorPtr> layers;
        DeclaratorPtr curd = d;
        while (curd) { layers.push_back(curd); if (curd->inner.has_value()) curd = curd->inner.value(); else break; }
        // Reverse each maximal run of adjacent Array layers (see the matching
        // comment in buildTypeFromDeclaration) so multidim dims nest correctly.
        for (size_t s = 0; s < layers.size(); ) {
            if (layers[s] && layers[s]->kind == Declarator::Kind::Array) {
                size_t e = s;
                while (e < layers.size() && layers[e] && layers[e]->kind == Declarator::Kind::Array) ++e;
                std::reverse(layers.begin() + s, layers.begin() + e);
                s = e;
            } else ++s;
        }
        for (int i = static_cast<int>(layers.size()) - 1; i >= 0; --i) {
            auto layer = layers[i];
            if (!layer) continue;
            if (layer->kind == Declarator::Kind::Pointer) {
                auto p = std::make_shared<TypeNode>();
                p->kind = TypeNode::Kind::Pointer;
                p->pointee = cur;
                p->ptrQual = layer->ptrQual;
                cur = p;
            } else if (layer->kind == Declarator::Kind::Array) {
                auto a = std::make_shared<TypeNode>();
                a->kind = TypeNode::Kind::Array;
                a->element = cur;
                a->arrayIsStar = layer->array.isStar;
                a->arrayIsStatic = layer->array.isStatic;
                a->arrayQual = layer->array.qual;
                if (layer->array.size.has_value()) a->sizeExpr = layer->array.size.value();
                if (a->arrayIsStar || (a->sizeExpr.has_value() && !ConstExprEvaluator::isIntegerConstantExpr(a->sizeExpr.value()))) {
                    if (!inParamPrototype) variablyModified = true;
                }
                cur = a;
            } else if (layer->kind == Declarator::Kind::Function) {
                auto f = std::make_shared<TypeNode>();
                f->kind = TypeNode::Kind::Function;
                f->element = cur;
                f->hasParamTypeList = layer->function.hasParamTypeList;
                f->isVariadic = layer->function.isVariadic;
                for (const auto &p : layer->function.params) {
                    std::shared_ptr<TypeNode> ptn = nullptr;
                    if (p.declarator) ptn = buildTypeFromDeclaration(DeclarationSpecifiers(), p.declarator, true, nullptr);
                    else if (p.typeSpec.has_value()) { ptn = std::make_shared<TypeNode>(); ptn->kind = TypeNode::Kind::Builtin; ptn->text = *p.typeSpec; }
                    if (!ptn) ptn = std::make_shared<TypeNode>();
                    f->params.push_back(ptn);
                }
                cur = f;
            }
        }
        if (outVM) *outVM = variablyModified;
        return cur;
    };

    // If there is a typedef-name specifier, prefer resolving it structurally
    for (const auto &ts : specs.typeSpecifiers) {
        if (ts.kind == DeclarationSpecifiers::TypeSpecifier::Kind::TypedefName) {
            std::unordered_set<std::string> visited;
            auto baseType = resolveTypedef(ts.text, visited);
            bool vm = false;
            auto applied = applyDeclaratorLayers(baseType, decl, false, &vm);
            return applied;
        }
    }

    // Fallback: build structured type from specs+decl
    return buildTypeFromDeclaration(specs, decl, false, nullptr);
}

void Semantic::recordDef(const std::string &name, const wvmcc::SourceSpan &span) {
    if (name.empty()) return;
    defCount[name]++;
    if (firstDefSpan.find(name) == firstDefSpan.end()) firstDefSpan[name] = span;
}

// ASTVisitor hooks overridden by Semantic
void Semantic::onIdent(const ASTVisitor::IdentifierExprPtr &id) {
    if (!id) return;
    if (!id->name.empty()) usedNames.insert(id->name);
}

void Semantic::onFunctionDef(const FunctionDefPtr &f) {
    if (!f) return;
    if (f->declarator) {
        // check compatibility with prior declarations
        std::string name = declaratorName(f->declarator);
        // 6.7.4p4: a function specifier (inline / _Noreturn) shall not appear in
        // a declaration of main.
        if (name == "main"
            && f->specifiers.funcSpecFlags != FunctionSpecifier::None
            && curDiagnostics) {
            Diagnostic diag;
            diag.severity = Diagnostic::Severity::Error;
            diag.message = "function specifier may not appear on 'main'";
            diag.span = f->declarator->span;
            curDiagnostics->push_back(std::move(diag));
        }
        // 6.9.1p5: in a function *definition*, each parameter shall be named —
        // except the lone `(void)` list, which denotes zero parameters.
        if (curDiagnostics && !f->params.empty()) {
            const auto &fps = f->params;
            bool voidParams = fps.size() == 1 && !fps[0].declarator && [&] {
                for (const auto &ts : fps[0].specifiers.typeSpecifiers)
                    if (ts.kind == DeclarationSpecifiers::TypeSpecifier::Kind::Simple
                        && ts.simple.size() == 1
                        && ts.simple[0] == DeclarationSpecifiers::SimpleTypeSpecifier::Void)
                        return true;
                return false;
            }();
            if (!voidParams) {
                for (const auto &p : fps) {
                    std::string pn = p.declarator ? declaratorName(p.declarator) : std::string();
                    if (pn.empty()) {
                        Diagnostic diag;
                        diag.severity = Diagnostic::Severity::Error;
                        diag.message = "parameter name omitted in function definition";
                        diag.span = f->declarator->span;
                        curDiagnostics->push_back(std::move(diag));
                        break;   // one diagnostic suffices
                    }
                }
            }
        }
        DeclarationPtr fake = std::make_shared<Declaration>();
        fake->specifiers = f->specifiers;
        fake->declarator = f->declarator;
        fake->initializer = std::nullopt;
        std::string sig = signatureForDeclaration(fake);
        // build canonical type representation and compare if available
        bool vm = false;
        auto tn = buildTypeFromDeclaration(f->specifiers, f->declarator, false, &vm);
        auto typeNode = tn;
        auto canonRepr = canonicalTypeRepr(f->specifiers, f->declarator);
        auto it = declaredSignatures.find(name);
            if (it != declaredSignatures.end() && it->second != sig && curDiagnostics) {
            std::string prev = it->second;
            int prevLine = -1;
            auto itspan = declaredSignatureSpan.find(name);
            if (itspan != declaredSignatureSpan.end()) prevLine = itspan->second.begin.line;
            auto itype = declaredTypeRepr.find(name);
            bool typesCompatible = itype != declaredTypeRepr.end()
                && redeclTypesCompatible(itype->second, canonRepr);
            // Qualifier-only differences first (redeclTypesCompatible ignores
            // top-level qualifiers, so it must not mask a qualifier mismatch).
            if (stripQualParts(prev) == stripQualParts(sig)) {
                Diagnostic diag;
                diag.severity = Diagnostic::Severity::Error;
                diag.message = "incompatible declaration for '" + name + "': qualifiers differ" + (prevLine>0 ? (" (previous at line " + std::to_string(prevLine) + ")") : std::string());
                diag.span = f->declarator->span;
                curDiagnostics->push_back(std::move(diag));
            } else if (typesCompatible) {
                // Signature strings differ but the types are compatible — e.g. a
                // prototyped definition completing an unprototyped declaration
                // `int f();` (C 6.2.7, 6.7.6.3p14). Adopt the prototyped form.
                if (canonRepr && canonRepr->kind == TypeNode::Kind::Function
                    && canonRepr->hasParamTypeList) {
                    declaredSignatures[name] = sig;
                    if (f->declarator) declaredSignatureSpan[name] = f->declarator->span;
                    declaredTypeRepr[name] = canonRepr;
                }
            } else if (itype != declaredTypeRepr.end()) {
                    Diagnostic diag;
                    diag.severity = Diagnostic::Severity::Error;
                    diag.message = "incompatible declaration for '" + name + "': type mismatch" + (prevLine > 0 ? (" (previous at line " + std::to_string(prevLine) + ")") : std::string());
                    diag.span = f->declarator->span;
                    curDiagnostics->push_back(std::move(diag));
            } else {
                    Diagnostic diag;
                    diag.severity = Diagnostic::Severity::Error;
                    diag.message = "incompatible declaration for '" + name + "'" + (prevLine>0 ? (" (previous at line " + std::to_string(prevLine) + ")") : std::string());
                    diag.span = f->declarator->span;
                    curDiagnostics->push_back(std::move(diag));
            }
        } else {
            declaredSignatures[name] = sig;
            if (f->declarator) declaredSignatureSpan.emplace(name, f->declarator->span);
            if (canonRepr) declaredTypeRepr[name] = canonRepr;
        }
        if (!f->specifiers.hasStorage(wvmcc::parser::StorageClass::Static)) recordDef(name, f->declarator->span);
        // Record file-scope function definition info for function-specifier rules
        if (functionDepth == 0) {
            auto &info = functionDecls[name];
            info.hasDef = true;
            if (f->specifiers.hasFuncSpec(FunctionSpecifier::Inline)) info.defIsInline = true;
            if (f->specifiers.hasStorage(wvmcc::parser::StorageClass::Extern)) info.externDecls++;
            if (f->specifiers.hasStorage(wvmcc::parser::StorageClass::Static)) info.staticDecls++;
        }
    }
}

void Semantic::onDeclaration(const DeclarationPtr &d) {
    if (!d) return;
    (void)d;
    // The signature-compatibility / declared-type / def-counting bookkeeping
    // below only applies at file scope. Inside a function, local declarations
    // have their own scope and must not collide with file-scope (or
    // each-other-across-functions) names. Block-scope declarations still need
    // their own diagnostics, though, so handle those here before returning.
    if (functionDepth > 0) {
        if (curDiagnostics) {
            // Block-scope declaration-time constraint checks (same as file
            // scope): tag-kind mismatch, bit-field widths, array sizes.
            checkTagKinds(d->specifiers, d->span, *curDiagnostics);
            checkBitfields(d->specifiers, *curDiagnostics);
            if (d->declarator) checkArraySizes(d->declarator, *curDiagnostics);
        }
        if (curDiagnostics && d->declarator) {
            // A variably-modified (VLA) type is permitted at block scope but
            // flagged as a warning.
            bool vm = false;
            buildTypeFromDeclaration(d->specifiers, d->declarator, false, &vm);
            if (vm) {
                Diagnostic diag;
                diag.severity = Diagnostic::Severity::Warning;
                diag.message = "declaration has variably-modified type";
                diag.span = d->declarator->span;
                curDiagnostics->push_back(std::move(diag));
            }
        }
        if (curDiagnostics) {
            // Block-scope external-linkage object with an initializer is an
            // error; block-scope `static` is fine (no linkage, C 6.2.2p6).
            if (d->initializer.has_value()
                && d->specifiers.hasStorage(wvmcc::parser::StorageClass::Extern)) {
                Diagnostic diag;
                diag.severity = Diagnostic::Severity::Error;
                diag.message = "declaration at block scope with external linkage shall not have an initializer";
                diag.span = d->span;
                curDiagnostics->push_back(std::move(diag));
            }
            // C 6.7.4p3: an inline definition with external linkage shall not
            // contain a definition of a modifiable object with static storage
            // duration. (A `const` static object is not modifiable → allowed.)
            if (inInlineExternalDef_
                && d->declarator
                && !isFunctionDeclarator(d->declarator)
                && d->specifiers.hasStorage(wvmcc::parser::StorageClass::Static)
                && !specsDeclObjectIsConst(d->specifiers, d->declarator)) {
                Diagnostic diag;
                diag.severity = Diagnostic::Severity::Error;
                diag.message = "inline definition with external linkage shall not "
                               "define a modifiable static-duration object";
                diag.span = d->span;
                curDiagnostics->push_back(std::move(diag));
            }
            // C 6.7.1: a block-scope function declaration shall have no explicit
            // storage-class specifier other than extern.
            if (d->declarator && isFunctionDeclarator(d->declarator)
                && !d->specifiers.hasStorage(wvmcc::parser::StorageClass::Extern)
                && (d->specifiers.hasStorage(wvmcc::parser::StorageClass::Static)
                    || d->specifiers.hasStorage(wvmcc::parser::StorageClass::Auto)
                    || d->specifiers.hasStorage(wvmcc::parser::StorageClass::Register))) {
                Diagnostic diag;
                diag.severity = Diagnostic::Severity::Error;
                diag.message = "function with block scope shall have no explicit storage-class";
                diag.span = d->span;
                curDiagnostics->push_back(std::move(diag));
            }
            // 6.7p7: a declared object (other than via a pointer, or an extern
            // declaration completed elsewhere) shall have a complete type. Reject
            // a block-scope object whose named struct/union tag is never defined.
            if (d->declarator && !isFunctionDeclarator(d->declarator)
                && !d->specifiers.hasStorage(wvmcc::parser::StorageClass::Typedef)
                && !d->specifiers.hasStorage(wvmcc::parser::StorageClass::Extern)) {
                auto objTy = canonicalTypeRepr(d->specifiers, d->declarator);
                if (!objTy) objTy = buildTypeFromDeclaration(d->specifiers, d->declarator, false, nullptr);
                if (objTy && (objTy->kind == TypeNode::Kind::Struct
                              || objTy->kind == TypeNode::Kind::Union)
                    && objTy->su && objTy->su->name.has_value()) {
                    bool complete = objTy->su->hasBody
                        || structUnionTagDefs.count(*objTy->su->name) != 0;
                    if (!complete) {
                        Diagnostic diag;
                        diag.severity = Diagnostic::Severity::Error;
                        diag.message = "variable has incomplete type '"
                            + (objTy->kind == TypeNode::Kind::Union ? std::string("union ")
                                                                    : std::string("struct "))
                            + *objTy->su->name + "'";
                        diag.span = d->span;
                        curDiagnostics->push_back(std::move(diag));
                    }
                }
            }
        }
        // Record the object in the current block scope and diagnose a
        // redefinition of an ordinary identifier in the same scope
        // (C 6.7p3 — no two declarations of the same identifier with no
        // linkage in the same scope, except as permitted). We only diagnose
        // objects (not typedefs, not extern/block-scope-function declarations
        // which have linkage) to stay conservative.
        if (d->declarator && !isFunctionDeclarator(d->declarator)
            && !d->specifiers.hasStorage(wvmcc::parser::StorageClass::Typedef)
            && !d->specifiers.hasStorage(wvmcc::parser::StorageClass::Extern)) {
            std::string nm = declaratorName(d->declarator);
            if (!nm.empty()) {
                LocalSym sym;
                bool vm2 = false;
                sym.type = canonicalTypeRepr(d->specifiers, d->declarator);
                if (!sym.type) sym.type = buildTypeFromDeclaration(d->specifiers, d->declarator, false, &vm2);
                sym.isConst = declarationObjectIsConst(d);
                sym.isRegister = d->specifiers.hasStorage(wvmcc::parser::StorageClass::Register);
                sym.span = d->declarator->span;
                if (!declareLocal(nm, sym) && curDiagnostics) {
                    Diagnostic diag;
                    diag.severity = Diagnostic::Severity::Error;
                    diag.message = "redefinition of '" + nm + "'";
                    diag.span = d->declarator->span;
                    curDiagnostics->push_back(std::move(diag));
                }
            }
        }
        return;
    }
    // perform a simple declaration compatibility check based on compact signature
    if (d->declarator) {
        std::string name = declaratorName(d->declarator);
        if (name.empty()) {
            if (curDiagnostics) {
                Diagnostic diag;
                diag.severity = Diagnostic::Severity::Error;
                diag.message = "unnamed declarator";
                diag.span = d->declarator->span;
                curDiagnostics->push_back(std::move(diag));
            }
            return;
        }
        std::string sig = signatureForDeclaration(d);
        bool vm = false;
        auto tn = buildTypeFromDeclaration(d->specifiers, d->declarator, false, &vm);
        if (vm && curDiagnostics) {
            Diagnostic diag;
            if (functionDepth == 0) {
                diag.severity = Diagnostic::Severity::Error;
                diag.message = "variably-modified type not allowed at file scope";
            } else {
                diag.severity = Diagnostic::Severity::Warning;
                diag.message = "declaration has variably-modified type";
            }
            diag.span = d->declarator->span;
            curDiagnostics->push_back(std::move(diag));
        }
        auto typeNode = tn;
        // produce canonical form (resolve typedefs) when possible
        auto canonRepr = canonicalTypeRepr(d->specifiers, d->declarator);
        auto it = declaredSignatures.find(name);
            if (it != declaredSignatures.end() && it->second != sig && curDiagnostics) {
            std::string prev = it->second;
            int prevLine = -1;
            auto itspan = declaredSignatureSpan.find(name);
            if (itspan != declaredSignatureSpan.end()) prevLine = itspan->second.begin.line;
            auto itype = declaredTypeRepr.find(name);
            bool typesCompatible = itype != declaredTypeRepr.end()
                && redeclTypesCompatible(itype->second, canonRepr);
            // Qualifier-only differences are checked first: redeclTypesCompatible
            // (like typeNodesEqual) ignores top-level qualifiers, so it must not
            // be allowed to mask a genuine `int x;` vs `const int x;` mismatch.
            if (stripQualParts(prev) == stripQualParts(sig)) {
                Diagnostic diag;
                diag.severity = Diagnostic::Severity::Error;
                diag.message = "incompatible declaration for '" + name + "': qualifiers differ" + (prevLine>0 ? (" (previous at line " + std::to_string(prevLine) + ")") : std::string());
                diag.span = d->declarator->span;
                curDiagnostics->push_back(std::move(diag));
            } else if (typesCompatible) {
                // Signature strings differ but the types are compatible — e.g. an
                // unprototyped `int f()` beside a prototype `int f(int,int)`
                // (C 6.2.7, 6.7.6.3p14). No diagnostic; adopt the prototyped form
                // as the composite type so a later definition matches it.
                if (canonRepr && canonRepr->kind == TypeNode::Kind::Function
                    && canonRepr->hasParamTypeList) {
                    declaredSignatures[name] = sig;
                    if (d->declarator) declaredSignatureSpan[name] = d->declarator->span;
                    declaredTypeRepr[name] = canonRepr;
                }
            } else if (itype != declaredTypeRepr.end()) {
                    Diagnostic diag;
                    diag.severity = Diagnostic::Severity::Error;
                    diag.message = "incompatible declaration for '" + name + "': type mismatch" + (prevLine > 0 ? (" (previous at line " + std::to_string(prevLine) + ")") : std::string());
                    diag.span = d->declarator->span;
                    curDiagnostics->push_back(std::move(diag));
            } else {
                    Diagnostic diag;
                    diag.severity = Diagnostic::Severity::Error;
                    diag.message = "incompatible declaration for '" + name + "'" + (prevLine>0 ? (" (previous at line " + std::to_string(prevLine) + ")") : std::string());
                    diag.span = d->declarator->span;
                    curDiagnostics->push_back(std::move(diag));
            }
        } else {
            declaredSignatures[name] = sig;
            if (d->declarator) declaredSignatureSpan.emplace(name, d->declarator->span);
            if (canonRepr) declaredTypeRepr[name] = canonRepr;
        }
    }
    
    if (d->declarator) {
        std::string rname = declaratorName(d->declarator);
        if (!rname.empty()) {
            bool isDef = false;
            if (d->specifiers.hasStorage(wvmcc::parser::StorageClass::Extern) && d->initializer.has_value()) isDef = true;
            if (!d->specifiers.hasStorage(wvmcc::parser::StorageClass::Extern)) isDef = true;
            // A bodyless function declaration (prototype) is NOT a definition,
            // even though it has no `extern`. Only function definitions (with
            // a compound-statement body) reach this code path through
            // onFunctionDef — a Declaration whose outermost declarator layer
            // is Function is always a prototype here.
            if (d->declarator && d->declarator->kind == Declarator::Kind::Function) {
                isDef = false;
            }
            // Compute canonical alignment and numeric value (if possible)
            auto [maybeVal, canon] = computeAlignFromSpecsTU(d->specifiers);
            std::string alignStr = canon;
            // A file-scope object with no initializer and no `extern` is a
            // *tentative* definition (6.9.2p2). Any number of them may appear,
            // and they may coexist with a single initialized definition, so
            // they must not be counted as external definitions for the
            // multiple-definitions check — but they still provide a definition.
            bool isTentative = isDef
                && !d->specifiers.hasStorage(wvmcc::parser::StorageClass::Extern)
                && !d->specifiers.hasStorage(wvmcc::parser::StorageClass::Static)
                && !d->initializer.has_value()
                && !(d->declarator && d->declarator->kind == Declarator::Kind::Function);
            // record definition marker
            if (isDef && !d->specifiers.hasStorage(wvmcc::parser::StorageClass::Static)) {
                if (isTentative) tentativeDefs.insert(rname);
                else recordDef(rname, d->declarator->span);
            }

            // C 6.9.2p3: a tentative definition with internal linkage (a
            // file-scope `static` object with no initializer) shall not have an
            // incomplete type. The named tag may be completed later in this TU,
            // so defer the completeness check to end of translation unit.
            if (d->specifiers.hasStorage(wvmcc::parser::StorageClass::Static)
                && !d->initializer.has_value()
                && d->declarator->kind != Declarator::Kind::Function) {
                auto objTy = canonicalTypeRepr(d->specifiers, d->declarator);
                if (!objTy) objTy = buildTypeFromDeclaration(d->specifiers, d->declarator, false, nullptr);
                if (objTy && (objTy->kind == TypeNode::Kind::Struct
                              || objTy->kind == TypeNode::Kind::Union)
                    && objTy->su && objTy->su->name.has_value() && !objTy->su->hasBody) {
                    pendingStaticTentativeTypes_.push_back(
                        {*objTy->su->name, objTy->kind == TypeNode::Kind::Union, d->span});
                }
            }

            if (!rname.empty()) {
                if (isDef) {
                    // check against prior recorded definition
                    auto its = seenAlign.find(rname);
                    if (its != seenAlign.end()) {
                        // if both are numeric, compare numerically
                        if (its->second.value.has_value() && maybeVal.has_value()) {
                            if (its->second.value.value() != *maybeVal) {
                                if (curDiagnostics) {
                                    Diagnostic diag; diag.severity = Diagnostic::Severity::Error;
                                    diag.message = "conflicting alignment specifier for '" + rname + "'";
                                    diag.span = d->span;
                                    curDiagnostics->push_back(std::move(diag));
                                }
                            }
                        } else if (its->second.canon != alignStr) {
                            // fallback string comparison
                            if (curDiagnostics) {
                                Diagnostic diag; diag.severity = Diagnostic::Severity::Error;
                                diag.message = "conflicting alignment specifier for '" + rname + "'";
                                diag.span = d->span;
                                curDiagnostics->push_back(std::move(diag));
                            }
                        }
                    } else {
                        seenAlign[rname] = {alignStr, maybeVal};
                        seenAlignSpan.emplace(rname, d->span);
                    }
                    if (!maybeVal.has_value()) {
                        if (its != seenAlign.end() && !its->second.canon.empty()) {
                            if (curDiagnostics) {
                                int prevLine = -1;
                                auto itspan = seenAlignSpan.find(rname);
                                if (itspan != seenAlignSpan.end()) prevLine = itspan->second.begin.line;
                                Diagnostic diag; diag.severity = Diagnostic::Severity::Error;
                                diag.message = "definition of '" + rname + "' has no alignment-specifier but prior declaration" + (prevLine>0 ? (" at line " + std::to_string(prevLine)) : std::string()) + " specifies alignment";
                                diag.span = d->span;
                                curDiagnostics->push_back(std::move(diag));
                            }
                        }
                    } else {
                        if (its != seenAlign.end()) {
                            if (its->second.value.has_value()) {
                                if (its->second.value.value() != *maybeVal) {
                                    if (curDiagnostics) {
                                        int prevLine = -1;
                                        auto itspan = seenAlignSpan.find(rname);
                                        if (itspan != seenAlignSpan.end()) prevLine = itspan->second.begin.line;
                                        Diagnostic diag; diag.severity = Diagnostic::Severity::Error;
                                        std::string prevVal = its->second.value.has_value() ? std::to_string(its->second.value.value()) : its->second.canon;
                                        std::string curVal = std::to_string(*maybeVal);
                                        diag.message = "definition of '" + rname + "' has alignment (" + curVal + ") not equivalent to prior declaration" + (prevLine>0 ? (" at line " + std::to_string(prevLine) + " (" + prevVal + ")") : std::string());
                                        diag.span = d->span;
                                        curDiagnostics->push_back(std::move(diag));
                                    }
                                }
                            } else if (its->second.canon != alignStr) {
                                if (curDiagnostics) {
                                    Diagnostic diag; diag.severity = Diagnostic::Severity::Error;
                                    diag.message = "definition of '" + rname + "' has alignment not equivalent to a prior declaration";
                                    diag.span = d->span;
                                    curDiagnostics->push_back(std::move(diag));
                                }
                            }
                        }
                    }
                } else {
                    // not a definition: compare against any recorded definition
                    auto itd = defAlign.find(rname);
                    if (itd != defAlign.end()) {
                        // if both sides have numeric values, compare numerically
                        if (itd->second.value.has_value() && maybeVal.has_value()) {
                            if (itd->second.value.value() != *maybeVal) {
                                if (curDiagnostics) {
                                    int defLine = -1;
                                    auto itdefspan = defAlignSpan.find(rname);
                                    if (itdefspan != defAlignSpan.end()) defLine = itdefspan->second.begin.line;
                                    std::string defVal = itd->second.value.has_value() ? std::to_string(itd->second.value.value()) : itd->second.canon;
                                    std::string curVal = maybeVal.has_value() ? std::to_string(*maybeVal) : alignStr;
                                    Diagnostic diag; diag.severity = Diagnostic::Severity::Error;
                                    diag.message = "declaration of '" + rname + "' specifies alignment (" + curVal + ") not equivalent to definition" + (defLine>0 ? (" at line " + std::to_string(defLine) + " (" + defVal + ")") : std::string());
                                    diag.span = d->span;
                                    curDiagnostics->push_back(std::move(diag));
                                }
                            }
                        } else if (itd->second.canon.empty() && !alignStr.empty()) {
                            if (curDiagnostics) {
                                int defLine = -1;
                                auto itdefspan = defAlignSpan.find(rname);
                                if (itdefspan != defAlignSpan.end()) defLine = itdefspan->second.begin.line;
                                Diagnostic diag; diag.severity = Diagnostic::Severity::Error;
                                diag.message = "declaration of '" + rname + "' specifies alignment but definition" + (defLine>0 ? (" at line " + std::to_string(defLine)) : std::string()) + " has none";
                                diag.span = d->span;
                                curDiagnostics->push_back(std::move(diag));
                            }
                        } else if (!itd->second.canon.empty() && !alignStr.empty() && itd->second.canon != alignStr) {
                                if (curDiagnostics) {
                                    int defLine = -1;
                                    auto itdefspan = defAlignSpan.find(rname);
                                    if (itdefspan != defAlignSpan.end()) defLine = itdefspan->second.begin.line;
                                    std::string defVal = itd->second.value.has_value() ? std::to_string(itd->second.value.value()) : itd->second.canon;
                                    Diagnostic diag; diag.severity = Diagnostic::Severity::Error;
                                    diag.message = "declaration of '" + rname + "' specifies alignment not equivalent to definition" + (defLine>0 ? (" at line " + std::to_string(defLine) + " (" + defVal + ")") : std::string());
                                    diag.span = d->span;
                                    curDiagnostics->push_back(std::move(diag));
                                }
                        }
                    } else {
                        // no definition yet: record seen alignment and conservatively
                        // error if multiple non-def declarations in this TU disagree
                        auto its = seenAlign.find(rname);
                        if (its == seenAlign.end()) {
                            seenAlign[rname] = {alignStr, maybeVal};
                        } else {
                            if (its->second.value.has_value() && maybeVal.has_value()) {
                                if (its->second.value.value() != *maybeVal) {
                                    if (curDiagnostics) {
                                        int prevLine = -1;
                                        auto itspan = seenAlignSpan.find(rname);
                                        if (itspan != seenAlignSpan.end()) prevLine = itspan->second.begin.line;
                                        std::string prevVal = its->second.value.has_value() ? std::to_string(its->second.value.value()) : its->second.canon;
                                        std::string curVal = maybeVal.has_value() ? std::to_string(*maybeVal) : alignStr;
                                        Diagnostic diag; diag.severity = Diagnostic::Severity::Error;
                                        diag.message = "conflicting alignment specifiers for '" + rname + "' in this translation unit: previous" + (prevLine>0 ? (" at line " + std::to_string(prevLine)) : std::string()) + " (" + prevVal + ") vs current (" + curVal + ")";
                                        diag.span = d->span;
                                        curDiagnostics->push_back(std::move(diag));
                                    }
                                }
                            } else if (!its->second.canon.empty() && !alignStr.empty() && its->second.canon != alignStr) {
                                if (curDiagnostics) {
                                    Diagnostic diag; diag.severity = Diagnostic::Severity::Error;
                                    diag.message = "conflicting alignment specifiers for '" + rname + "' in this translation unit";
                                    diag.span = d->span;
                                    curDiagnostics->push_back(std::move(diag));
                                }
                            }
                        }
                    }
                }
            }

            // If this is a file-scope function declaration, update summary counts
            if (functionDepth == 0 && isFunctionDeclarator(d->declarator)) {
                auto &info = functionDecls[rname];
                info.totalDecls++;
                if (d->specifiers.hasFuncSpec(FunctionSpecifier::Inline)) info.inlineDecls++;
                if (d->specifiers.hasStorage(wvmcc::parser::StorageClass::Extern)) info.externDecls++;
                if (d->specifiers.hasStorage(wvmcc::parser::StorageClass::Static)) info.staticDecls++;
            }
        }
    }

    // If this declaration is inside a function body (block scope) and has
    // external linkage with an initializer, report error. Block-scope `static`
    // gives the identifier no linkage (C 6.2.2p6) and is allowed to have an
    // initializer.
    if (functionDepth > 0 && curDiagnostics) {
        if (d->initializer.has_value() && d->specifiers.hasStorage(wvmcc::parser::StorageClass::Extern)) {
            Diagnostic diag;
            diag.severity = Diagnostic::Severity::Error;
            diag.message = "declaration at block scope with external linkage shall not have an initializer";
            diag.span = d->span;
            curDiagnostics->push_back(std::move(diag));
        }
        // C 6.7.1: a declaration of an identifier for a function that has block
        // scope shall have no explicit storage-class specifier other than extern.
        if (d->declarator && isFunctionDeclarator(d->declarator) && !d->specifiers.hasStorage(wvmcc::parser::StorageClass::Extern)) {
            if (d->specifiers.hasStorage(wvmcc::parser::StorageClass::Static) || d->specifiers.hasStorage(wvmcc::parser::StorageClass::Auto) || d->specifiers.hasStorage(wvmcc::parser::StorageClass::Register)) {
                Diagnostic diag;
                diag.severity = Diagnostic::Severity::Error;
                diag.message = "function with block scope shall have no explicit storage-class";
                diag.span = d->span;
                curDiagnostics->push_back(std::move(diag));
            }
        }
    }
}

// --- Local (block-scope) symbol table helpers -----------------------------

const Semantic::LocalSym *Semantic::lookupLocal(const std::string &name) const {
    for (auto it = localScopes.rbegin(); it != localScopes.rend(); ++it) {
        auto f = it->find(name);
        if (f != it->end()) return &f->second;
    }
    return nullptr;
}

bool Semantic::declareLocal(const std::string &name, const LocalSym &sym) {
    if (localScopes.empty()) return true; // no active scope (shouldn't happen)
    auto &cur = localScopes.back();
    if (cur.find(name) != cur.end()) return false; // already in current scope
    cur.emplace(name, sym);
    return true;
}

// True if `specs`+`declarator` declare a top-level `const`-qualified *scalar/
// aggregate object* whose stored value cannot be modified through its name.
// We are deliberately conservative: a declarator that involves a pointer,
// array, or function layer is NOT treated as a const object here, because the
// specifier-level const then qualifies a pointee/element, not the named object
// itself (e.g. `const int *p` is a modifiable pointer to const int; `const int
// a[3]` is an array, which is non-assignable for a separate reason). This keeps
// the const-assignment diagnostic free of false positives on valid code.
static bool specsDeclObjectIsConst(const DeclarationSpecifiers &specs, const DeclaratorPtr &declarator) {
    if (!specs.hasTypeQual(TypeQualifier::Const)) return false;
    DeclaratorPtr cur = declarator;
    while (cur) {
        if (cur->kind == Declarator::Kind::Pointer
            || cur->kind == Declarator::Kind::Function
            || cur->kind == Declarator::Kind::Array) {
            return false;
        }
        if (cur->inner.has_value()) cur = cur->inner.value(); else break;
    }
    return true;
}

static bool declarationObjectIsConst(const DeclarationPtr &d) {
    if (!d) return false;
    return specsDeclObjectIsConst(d->specifiers, d->declarator);
}

void Semantic::onEnterFunction(const FunctionDefPtr &f) {
    functionDepth++;
    // C 6.7.4p7: a definition is an *inline definition* when every file-scope
    // declaration is `inline` without `extern`; such a definition has external
    // linkage unless `static`. We approximate from this definition's own
    // specifiers: inline, and neither static nor extern.
    inInlineExternalDef_ = f
        && f->specifiers.hasFuncSpec(wvmcc::parser::FunctionSpecifier::Inline)
        && !f->specifiers.hasStorage(wvmcc::parser::StorageClass::Static)
        && !f->specifiers.hasStorage(wvmcc::parser::StorageClass::Extern);
    // Open a scope for the function's parameters and record their types so
    // typeOfExpr can resolve parameter identifiers used in the body.
    localScopes.emplace_back();
    if (f) {
        for (const auto &p : f->params) {
            if (!p.declarator) continue;
            std::string pn = declaratorName(p.declarator);
            if (pn.empty()) continue;
            LocalSym sym;
            sym.type = canonicalTypeRepr(p.specifiers, p.declarator);
            if (!sym.type) sym.type = buildTypeFromDeclaration(p.specifiers, p.declarator, true, nullptr);
            sym.isConst = specsDeclObjectIsConst(p.specifiers, p.declarator);
            sym.span = p.declarator->span;
            // params share the function body's top scope; overwrite duplicates
            localScopes.back()[pn] = sym;
        }
    }
}

void Semantic::onExitFunction(const FunctionDefPtr &f) {
    (void)f;
    inInlineExternalDef_ = false;
    if (functionDepth > 0) --functionDepth;
    if (!localScopes.empty()) localScopes.pop_back();
}

void Semantic::onEnterBlock() {
    localScopes.emplace_back();
}

void Semantic::onExitBlock() {
    if (!localScopes.empty()) localScopes.pop_back();
}

// Forward declarations of the type-classification helpers (defined in the
// anonymous namespace lower in this TU) so the statement/expression hooks above
// their definition can use them. All unnamed namespaces in a TU are the same
// namespace, so these resolve to the same internal-linkage functions.
namespace {
bool tcIsVoid(const std::shared_ptr<TypeNode> &t);
bool tcIsStructOrUnion(const std::shared_ptr<TypeNode> &t);
bool tcIsArray(const std::shared_ptr<TypeNode> &t);
}

void Semantic::onExpr(const ExprPtr &e) {
    if (!e || !curDiagnostics) return;
    // Drive expression-level constraint diagnostics by computing the type of
    // the full expression (typeOfExpr recurses and emits diagnostics for any
    // ill-formed subexpression it encounters).
    (void)typeOfExpr(e);
}

void Semantic::onStmt(const StmtPtr &s) {
    if (!s || !curDiagnostics) return;
    // The controlling expression of a selection/iteration statement shall have
    // scalar type (6.8.4.1p1, 6.8.5p2). Reject only a positively non-scalar
    // (struct/union/array/void) controlling expression.
    auto checkScalarCtrl = [&](const ExprPtr &cond, const char *what) {
        if (!cond) return;
        auto t = typeOfExpr(cond).type;
        if (tcIsStructOrUnion(t) || tcIsArray(t) || tcIsVoid(t)) {
            Diagnostic d; d.severity = Diagnostic::Severity::Error;
            d.message = std::string("controlling expression of '") + what
                      + "' must have scalar type";
            d.span = cond->span;
            curDiagnostics->push_back(std::move(d));
        }
    };
    switch (s->kind) {
        case Stmt::Kind::If:
            checkScalarCtrl(std::static_pointer_cast<IfStmt>(s)->cond, "if");
            break;
        case Stmt::Kind::While:
            checkScalarCtrl(std::static_pointer_cast<WhileStmt>(s)->cond, "while");
            break;
        case Stmt::Kind::DoWhile:
            checkScalarCtrl(std::static_pointer_cast<DoWhileStmt>(s)->cond, "do");
            break;
        case Stmt::Kind::For: {
            auto fs = std::static_pointer_cast<ForStmt>(s);
            if (fs->cond) checkScalarCtrl(*fs->cond, "for");
            // 6.8.5p3: a declaration in the for clause-1 shall declare only
            // identifiers with storage class auto or register.
            if (fs->init && std::holds_alternative<DeclarationPtr>((*fs->init)->item)) {
                auto d = std::get<DeclarationPtr>((*fs->init)->item);
                if (d && (d->specifiers.hasStorage(StorageClass::Static)
                          || d->specifiers.hasStorage(StorageClass::Extern)
                          || d->specifiers.hasStorage(StorageClass::ThreadLocal)
                          || d->specifiers.hasStorage(StorageClass::Typedef))) {
                    Diagnostic diag; diag.severity = Diagnostic::Severity::Error;
                    diag.message = "declaration in 'for' loop clause-1 may only "
                                   "declare auto or register objects";
                    diag.span = d->span;
                    curDiagnostics->push_back(std::move(diag));
                }
            }
            break;
        }
        default: break;
    }
}

bool Semantic::run(std::vector<wvmcc::Diagnostic> &diagnostics) {
    if (!tu_) return true;
    // Collect file-scope names that decay to an address constant (arrays and
    // functions) before any checks, so the static-initializer-constant check
    // can recognise `static int *p = arr;` / `static fn_t f = func;`.
    addressConstantNames_.clear();
    for (auto &ext : tu_->externals) {
        if (!ext) continue;
        if (auto fd = std::get_if<FunctionDefPtr>(&ext->decl)) {
            if (*fd && (*fd)->declarator) addressConstantNames_.insert(declaratorName((*fd)->declarator));
        } else if (auto dp = std::get_if<DeclarationPtr>(&ext->decl)) {
            if (*dp && (*dp)->declarator) {
                auto k = (*dp)->declarator->kind;
                if (k == Declarator::Kind::Array || k == Declarator::Kind::Function)
                    addressConstantNames_.insert(declaratorName((*dp)->declarator));
            }
        }
    }

    // First pass: per-external checks (tags/enums, storage-class constraints,
    // collect internal (static) definitions for duplicate checking)
    internalDefs.clear();
    for (auto &ext : tu_->externals) {
        checkExternal(ext, diagnostics);
    }

    // clear any previous state for external def/use collection
    defCount.clear();
    firstDefSpan.clear();
    tentativeDefs.clear();
    usedNames.clear();
    declaredSignatures.clear();

    // Traverse the whole translation unit to collect defs and uses via ASTVisitor hooks
    // set diagnostics pointer so hooks can emit diagnostics while traversing
    curDiagnostics = &diagnostics;
    traverseTranslationUnit(tu_);
    curDiagnostics = nullptr;

    // Check duplicate external definitions (multiple external defs for same name)
    for (const auto &p : defCount) {
        const auto &name = p.first;
        int count = p.second;
        if (count > 1) {
            Diagnostic diag;
            diag.severity = Diagnostic::Severity::Error;
            diag.message = "multiple external definitions for '" + name + "'";
            diag.span = firstDefSpan[name];
            diagnostics.push_back(std::move(diag));
        }
    }

    // C 6.9.2p3: a file-scope `static` tentative definition must not name an
    // incomplete type. Now that the whole TU has been traversed, a tag still
    // absent from structUnionTagDefs is never completed → diagnose.
    for (const auto &p : pendingStaticTentativeTypes_) {
        if (structUnionTagDefs.count(p.tag) == 0) {
            Diagnostic diag;
            diag.severity = Diagnostic::Severity::Error;
            diag.message = "tentative definition has incomplete type '"
                + std::string(p.isUnion ? "union " : "struct ") + p.tag + "'";
            diag.span = p.span;
            diagnostics.push_back(std::move(diag));
        }
    }

    // Warn about identifier uses with no external definition in TU
    for (const auto &name : usedNames) {
        auto it = defCount.find(name);
        int count = (it == defCount.end()) ? 0 : it->second;
        // A tentative definition provides an external definition (it becomes a
        // zero-initialized definition at end of translation unit, 6.9.2p2).
        if (count == 0 && tentativeDefs.find(name) == tentativeDefs.end()) {
            // If all file-scope declarations for this function are inline (no extern),
            // they do not provide an external definition and we should not warn here.
            if (functionDecls.find(name) != functionDecls.end()) {
                const auto &info = functionDecls[name];
                if (info.inlineDecls > 0 && info.externDecls == 0 && info.staticDecls == 0) {
                    continue;
                }
            }
            Diagnostic diag;
            diag.severity = Diagnostic::Severity::Warning;
            diag.message = "identifier '" + name + "' used but no external definition in this translation unit";
            diagnostics.push_back(std::move(diag));
        }
    }

    // After traversal, compute inline-only names and emit diagnostics for
    // functions declared inline with extern declarations but lacking a
    // definition in this translation unit.
    inlineOnlyNames.clear();
    for (const auto &p : functionDecls) {
        const auto &name = p.first;
        const auto &info = p.second;
        if (info.totalDecls > 0 && info.inlineDecls > 0 && info.externDecls == 0 && info.staticDecls == 0) {
            inlineOnlyNames.insert(name);
        }
        // If the function has inline declarations and also extern declarations,
        // require a definition in this TU (conservative enforcement)
        if (info.inlineDecls > 0 && info.externDecls > 0 && !info.hasDef) {
            Diagnostic diag;
            diag.severity = Diagnostic::Severity::Error;
            diag.message = "function '" + name + "' declared inline with external linkage must be defined in this translation unit";
            // try to provide a span from any prior declaration
            if (declaredSignatures.find(name) != declaredSignatures.end()) {
                // no span available here; emit without span
            }
            diagnostics.push_back(std::move(diag));
        }
    }

    // Check internal (static) defs for duplicate definitive definitions
    for (const auto &p : internalDefs) {
        // if definitive count >1 we would have emitted earlier while building internalDefs
        (void)p;
    }
    // Determine whether any Error diagnostics were appended by this pass.
    bool hasError = false;
    for (const auto &d : diagnostics) {
        if (d.severity == Diagnostic::Severity::Error) { hasError = true; break; }
    }

    return !hasError;
}

// forward declare helper
static void processTypeSpecifiersForTags(const DeclarationSpecifiers &specs, std::unordered_map<std::string, wvmcc::SourceSpan> &structDefs, std::unordered_map<std::string, wvmcc::SourceSpan> &enumDefs, const SourceSpan &span, std::vector<wvmcc::Diagnostic> &diagnostics, std::unordered_set<const void*> &seenSuDefs);

void Semantic::checkExternal(const ExternalDeclPtr &e, std::vector<wvmcc::Diagnostic> &diagnostics) {
    if (!e) return;
    if (std::holds_alternative<FunctionDefPtr>(e->decl)) {
        auto f = std::get<FunctionDefPtr>(e->decl);
        // record any struct/union/enum definitions appearing in function specifiers
        processTypeSpecifiersForTags(f->specifiers, structUnionTagDefs, enumTagDefs, f->span, diagnostics, seenSuDefs_);
        checkTagKinds(f->specifiers, f->span, diagnostics);
        checkBitfields(f->specifiers, diagnostics);
        checkFunction(f, diagnostics);
    } else if (std::holds_alternative<DeclarationPtr>(e->decl)) {
        auto d = std::get<DeclarationPtr>(e->decl);
        // inspect declaration specifiers for tag definitions
        processTypeSpecifiersForTags(d->specifiers, structUnionTagDefs, enumTagDefs, d->span, diagnostics, seenSuDefs_);
        checkTagKinds(d->specifiers, d->span, diagnostics);
        checkBitfields(d->specifiers, diagnostics);
        checkDeclaration(d, diagnostics);
    }
}

// Helper to inspect type-specifiers in a declaration or function specifiers to
// record or detect duplicate struct/union and enum tag definitions.
static void processTypeSpecifiersForTags(const DeclarationSpecifiers &specs, std::unordered_map<std::string, wvmcc::SourceSpan> &structDefs, std::unordered_map<std::string, wvmcc::SourceSpan> &enumDefs, const SourceSpan &span, std::vector<wvmcc::Diagnostic> &diagnostics, std::unordered_set<const void*> &seenSuDefs) {
    for (const auto &ts : specs.typeSpecifiers) {
        if (ts.kind == DeclarationSpecifiers::TypeSpecifier::Kind::StructOrUnion) {
            if (ts.su && ts.su->name) {
                if (ts.su->hasBody) {
                    // Skip if this is a forward reference sharing the same su pointer as
                    // a previously-registered definition (parser reuses the registered pointer).
                    if (seenSuDefs.count(ts.su.get()) > 0) continue;
                    seenSuDefs.insert(ts.su.get());
                    const std::string &tag = *ts.su->name;
                    auto it = structDefs.find(tag);
                    if (it != structDefs.end()) {
                        Diagnostic d; d.severity = Diagnostic::Severity::Error; d.message = "duplicate tag definition for '" + tag + "'"; d.span = span; diagnostics.push_back(std::move(d));
                    } else {
                        structDefs[tag] = span;
                    }
                }
            }
        } else if (ts.kind == DeclarationSpecifiers::TypeSpecifier::Kind::Enum) {
            if (ts.en && ts.en->name) {
                if (ts.en->hasBody) {
                    const std::string &tag = *ts.en->name;
                    auto it = enumDefs.find(tag);
                    if (it != enumDefs.end()) {
                        Diagnostic d; d.severity = Diagnostic::Severity::Error; d.message = "duplicate enum tag definition for '" + tag + "'"; d.span = span; diagnostics.push_back(std::move(d));
                    } else {
                        enumDefs[tag] = span;
                    }
                }
            }
        } else if (ts.kind == DeclarationSpecifiers::TypeSpecifier::Kind::Atomic) {
            // If this is `_Atomic(inner)`, recurse into inner specs to find any tag definitions
            if (ts.atomicInner) {
                processTypeSpecifiersForTags(*ts.atomicInner, structDefs, enumDefs, span, diagnostics, seenSuDefs);
            }
        }
            // If this enum has a body, validate enumerators: duplicate names and constant inits
            if (ts.kind == DeclarationSpecifiers::TypeSpecifier::Kind::Enum) {
                if (ts.en && ts.en->hasBody) {
                    std::unordered_map<std::string, long long> seenVals;
                    long long next = 0;
                    for (const auto &e : ts.en->enumerators) {
                        if (seenVals.find(e.name) != seenVals.end()) {
                            Diagnostic d; d.severity = Diagnostic::Severity::Error; d.message = "duplicate enumerator '" + e.name + "'"; d.span = span; diagnostics.push_back(std::move(d));
                            continue;
                        }
                        if (e.value) {
                            auto v = ConstExprEvaluator::evalIntegerConstantExpr(e.value.value());
                            if (!v.has_value()) {
                                Diagnostic d; d.severity = Diagnostic::Severity::Error; d.message = "enumerator value must be an integer constant expression"; d.span = e.value.value()->span; diagnostics.push_back(std::move(d));
                            } else if (*v < -2147483648LL || *v > 2147483647LL) {
                                // C 6.7.2.2p2: an enumeration constant's value must
                                // be representable as an `int`. (Also catches an
                                // int-typed constant expression that overflows,
                                // 6.6p4 — e.g. `2147483647 + 1`.)
                                Diagnostic d; d.severity = Diagnostic::Severity::Error; d.message = "enumerator value " + std::to_string(*v) + " is not representable as 'int'"; d.span = e.value.value()->span; diagnostics.push_back(std::move(d));
                            } else {
                                seenVals[e.name] = *v;
                                next = *v + 1;
                            }
                        } else {
                            seenVals[e.name] = next;
                            next++;
                        }
                    }
                }
            }
            if (ts.kind == DeclarationSpecifiers::TypeSpecifier::Kind::Atomic) {
                if (ts.atomicInner) {
                    // validate any enums inside the atomic inner specifier as well
                    for (const auto &its : ts.atomicInner->typeSpecifiers) {
                        if (its.kind == DeclarationSpecifiers::TypeSpecifier::Kind::Enum) {
                            if (its.en && its.en->hasBody) {
                                std::unordered_map<std::string, long long> seenVals;
                                long long next = 0;
                                for (const auto &e : its.en->enumerators) {
                                    if (seenVals.find(e.name) != seenVals.end()) {
                                        Diagnostic d; d.severity = Diagnostic::Severity::Error; d.message = "duplicate enumerator '" + e.name + "'"; d.span = span; diagnostics.push_back(std::move(d));
                                        continue;
                                    }
                                    if (e.value) {
                                        auto v = ConstExprEvaluator::evalIntegerConstantExpr(e.value.value());
                                        if (!v.has_value()) {
                                            Diagnostic d; d.severity = Diagnostic::Severity::Error; d.message = "enumerator value must be an integer constant expression"; d.span = e.value.value()->span; diagnostics.push_back(std::move(d));
                                        } else {
                                            seenVals[e.name] = *v;
                                            next = *v + 1;
                                        }
                                    } else {
                                        seenVals[e.name] = next;
                                        next++;
                                    }
                                }
                            }
                        }
                    }
                }
            }
    }
}

void Semantic::checkTagKinds(const DeclarationSpecifiers &specs, const wvmcc::SourceSpan &span, std::vector<wvmcc::Diagnostic> &diagnostics) {
    // Record/verify the kind associated with each *named* struct/union/enum tag.
    // C 6.7.2.3p2: a given tag may not be declared as two different kinds.
    auto note = [&](const std::string &name, char kind, const char *kindWord) {
        if (name.empty()) return;
        auto it = tagKinds_.find(name);
        if (it == tagKinds_.end()) {
            tagKinds_[name] = kind;
            return;
        }
        if (it->second != kind) {
            const char *prevWord = it->second == 's' ? "struct" : (it->second == 'u' ? "union" : "enum");
            Diagnostic diag; diag.severity = Diagnostic::Severity::Error;
            diag.message = std::string("'") + name + "' defined as wrong kind of tag ('"
                           + kindWord + "' but previously declared as '" + prevWord + "')";
            diag.span = span;
            diagnostics.push_back(std::move(diag));
        }
    };
    for (const auto &ts : specs.typeSpecifiers) {
        if (ts.kind == DeclarationSpecifiers::TypeSpecifier::Kind::StructOrUnion) {
            if (ts.su && ts.su->name) {
                bool isUnion = ts.su->kind == StructOrUnionSpecifier::Kind::Union;
                note(*ts.su->name, isUnion ? 'u' : 's', isUnion ? "union" : "struct");
            }
        } else if (ts.kind == DeclarationSpecifiers::TypeSpecifier::Kind::Enum) {
            if (ts.en && ts.en->name) note(*ts.en->name, 'e', "enum");
        } else if (ts.kind == DeclarationSpecifiers::TypeSpecifier::Kind::Atomic) {
            if (ts.atomicInner) checkTagKinds(*ts.atomicInner, span, diagnostics);
        }
    }
}

// Width in bits of a (signed/unsigned) integer member type for bit-field
// upper-bound checking, given the LP64 data model. Returns 0 when the type is
// not a recognised integer type (in which case the caller skips the upper
// bound check to stay conservative).
static int bitfieldTypeBits(const DeclarationSpecifiers &specs) {
    using S = DeclarationSpecifiers::SimpleTypeSpecifier;
    if (specs.typeSpecifiers.empty()) return 32; // bare `int` bit-field
    const auto &ts = specs.typeSpecifiers.front();
    if (ts.kind != DeclarationSpecifiers::TypeSpecifier::Kind::Simple) return 0;
    int longCount = 0; bool hasChar=false, hasShort=false, hasInt=false, hasBool=false;
    for (auto st : ts.simple) {
        switch (st) {
            case S::Char: hasChar = true; break;
            case S::Short: hasShort = true; break;
            case S::Int: hasInt = true; break;
            case S::Long: longCount++; break;
            case S::Bool: hasBool = true; break;
            case S::Signed: case S::Unsigned: break; // signedness: no width effect
            default: return 0; // float/double/void/etc: not a valid bit-field base
        }
    }
    if (hasBool) return 1;
    if (hasChar) return 8;
    if (hasShort) return 16;
    if (longCount >= 1) return 64; // long / long long are 64-bit (LP64)
    if (hasInt || (hasChar==false && hasShort==false)) return 32; // int (incl. signed/unsigned alone)
    return 32;
}

void Semantic::checkBitfields(const DeclarationSpecifiers &specs, std::vector<wvmcc::Diagnostic> &diagnostics) {
    // Guard against cycles in self-/mutually-referential aggregates (e.g.
    // `struct node { struct node *next; }`): visit each specifier pointer once.
    std::unordered_set<const void*> visited;
    std::function<void(const std::shared_ptr<StructOrUnionSpecifier> &)> visitSU;
    visitSU = [&](const std::shared_ptr<StructOrUnionSpecifier> &su) {
        if (!su || !su->hasBody) return;
        if (!visited.insert(su.get()).second) return; // already visited (cycle)
        for (const auto &member : su->members) {
            // Recurse only into *nested* aggregate definitions (a member whose
            // specifier itself carries a body — e.g. an anonymous nested
            // struct/union). A member that merely *references* a tag (such as a
            // pointer `struct node *next`) is not descended into: doing so would
            // loop forever on self-/mutually-referential types.
            for (const auto &mts : member.specifiers.typeSpecifiers) {
                if (mts.kind == DeclarationSpecifiers::TypeSpecifier::Kind::StructOrUnion
                    && mts.su && mts.su->hasBody)
                    visitSU(mts.su);
            }
            // C 6.7.2.1p5: a bit-field's type shall be a (qualified or
            // unqualified) version of _Bool, signed int, unsigned int, or some
            // other implementation-defined type. We only positively reject a
            // floating-point base type (float/double), which is never allowed;
            // other integer-ish types are left accepted to avoid false
            // positives on implementation-defined extensions.
            bool memberIsFloat = false;
            if (!member.specifiers.typeSpecifiers.empty()
                && member.specifiers.typeSpecifiers.front().kind == DeclarationSpecifiers::TypeSpecifier::Kind::Simple) {
                using S = DeclarationSpecifiers::SimpleTypeSpecifier;
                for (auto st : member.specifiers.typeSpecifiers.front().simple) {
                    if (st == S::Float || st == S::Double) { memberIsFloat = true; break; }
                }
            }
            for (const auto &sd : member.declarators) {
                if (!sd.bitfieldWidth.has_value() || !sd.bitfieldWidth.value()) continue;
                auto widthExpr = sd.bitfieldWidth.value();
                if (memberIsFloat) {
                    Diagnostic diag; diag.severity = Diagnostic::Severity::Error;
                    diag.message = "bit-field has non-integral type";
                    diag.span = widthExpr->span;
                    diagnostics.push_back(std::move(diag));
                    continue;
                }
                auto v = ConstExprEvaluator::evalIntegerConstantExpr(widthExpr);
                if (!v.has_value()) {
                    Diagnostic diag; diag.severity = Diagnostic::Severity::Error;
                    diag.message = "bit-field width is not an integer constant expression";
                    diag.span = widthExpr->span;
                    diagnostics.push_back(std::move(diag));
                    continue;
                }
                long long w = *v;
                bool named = sd.declarator && !sd.declarator->id.name.empty();
                if (w < 0) {
                    Diagnostic diag; diag.severity = Diagnostic::Severity::Error;
                    diag.message = "bit-field has negative width";
                    diag.span = widthExpr->span;
                    diagnostics.push_back(std::move(diag));
                    continue;
                }
                if (w == 0 && named) {
                    // A zero-width bit-field shall have no declarator (C 6.7.2.1p4).
                    Diagnostic diag; diag.severity = Diagnostic::Severity::Error;
                    diag.message = "named bit-field has zero width";
                    diag.span = widthExpr->span;
                    diagnostics.push_back(std::move(diag));
                    continue;
                }
                int bits = bitfieldTypeBits(member.specifiers);
                if (bits > 0 && w > bits) {
                    Diagnostic diag; diag.severity = Diagnostic::Severity::Error;
                    diag.message = "width of bit-field exceeds width of its type";
                    diag.span = widthExpr->span;
                    diagnostics.push_back(std::move(diag));
                }
            }
        }
    };
    for (const auto &ts : specs.typeSpecifiers) {
        if (ts.kind == DeclarationSpecifiers::TypeSpecifier::Kind::StructOrUnion && ts.su)
            visitSU(ts.su);
        else if (ts.kind == DeclarationSpecifiers::TypeSpecifier::Kind::Atomic && ts.atomicInner)
            checkBitfields(*ts.atomicInner, diagnostics);
    }
}

void Semantic::checkDeclaration(const DeclarationPtr &d, std::vector<wvmcc::Diagnostic> &diagnostics) {
    if (!d) return;
    // Struct/union body constraints apply even to a bare type definition with no
    // declarator (`struct node { struct node next; };`), so check before the
    // no-declarator early return.
    checkStructSelfContainment(d->specifiers, d->span, diagnostics);
    if (!d->declarator) {
        if (verbose_) {
            Diagnostic diag;
            diag.severity = Diagnostic::Severity::Warning;
            diag.message = "declaration without declarator";
            diag.span = d->span;
            diagnostics.push_back(std::move(diag));
        }
        return;
    }
    // If the declarator denotes a function type, applying type qualifiers to
    // the function type is undefined (C 6.7.3.10). Report an error if any
    // type qualifiers are present on the function type specifier-list.
    if (d->declarator && isFunctionDeclarator(d->declarator)) {
        if (d->specifiers.typeQualFlags != TypeQualifier::None) {
            Diagnostic diag;
            diag.severity = Diagnostic::Severity::Error;
            diag.message = "type qualifiers on function type are undefined behavior";
            diag.span = d->span;
            diagnostics.push_back(std::move(diag));
        }
    }
    if (declaratorName(d->declarator).empty()) {
        Diagnostic diag;
        diag.severity = Diagnostic::Severity::Error;
        diag.message = "unnamed declarator";
        diag.span = d->declarator->span;
        diagnostics.push_back(std::move(diag));
    }
    // 6.7.2p2: the type-specifier multiset must be a valid combination.
    if (std::string msg = invalidSimpleTypeMultiset(d->specifiers); !msg.empty()) {
        Diagnostic diag;
        diag.severity = Diagnostic::Severity::Error;
        diag.message = msg;
        diag.span = d->span;
        diagnostics.push_back(std::move(diag));
    }
    // 6.7.6.2p1 / 6.7.6.3p1: no function returning array/function, no array of
    // functions.
    if (std::string msg = illegalFuncArrayCombo(d->declarator); !msg.empty()) {
        Diagnostic diag;
        diag.severity = Diagnostic::Severity::Error;
        diag.message = msg;
        diag.span = d->declarator->span;
        diagnostics.push_back(std::move(diag));
    }
    // C 6.7.6.2p1: array size must be > 0 when it is a constant expression.
    checkArraySizes(d->declarator, diagnostics);
    // External-level semantic checks: storage-class constraints
    if (d->specifiers.hasStorage(wvmcc::parser::StorageClass::Auto) || d->specifiers.hasStorage(wvmcc::parser::StorageClass::Register)) {
        Diagnostic diag;
        diag.severity = Diagnostic::Severity::Error;
        if (d->specifiers.hasStorage(wvmcc::parser::StorageClass::Auto)) diag.message = "storage-class specifier 'auto' is not allowed in external declarations";
        else diag.message = "storage-class specifier 'register' is not allowed in external declarations";
        diag.span = d->declarator->span;
        diagnostics.push_back(std::move(diag));
    }

    // 6.7.9p4: every expression in the initializer of an object with static or
    // thread storage duration shall be a constant expression or string literal.
    // checkDeclaration only sees file-scope declarations, where every object has
    // static storage duration regardless of the `static` keyword — so the rule
    // applies to any initialized object that is not a typedef or function.
    if (d->initializer.has_value()
        && !d->specifiers.hasStorage(wvmcc::parser::StorageClass::Typedef)
        && !isFunctionDeclarator(d->declarator)) {
        if (!initializerIsConstant(d->initializer.value(), diagnostics)) {
            Diagnostic diag;
            diag.severity = Diagnostic::Severity::Error;
            diag.message = "initializer for object with static storage duration must be constant expression or string literal";
            diag.span = d->span;
            diagnostics.push_back(std::move(diag));
        }
    }

    // 6.7.4p2: function specifiers (inline, _Noreturn) shall appear only in a
    // function declaration. Applying one to an object is a constraint violation.
    if (d->specifiers.funcSpecFlags != FunctionSpecifier::None
        && !isFunctionDeclarator(d->declarator)
        && !d->specifiers.hasStorage(wvmcc::parser::StorageClass::Typedef)) {
        Diagnostic diag;
        diag.severity = Diagnostic::Severity::Error;
        diag.message = "function specifier may not appear on a non-function declaration";
        diag.span = d->span;
        diagnostics.push_back(std::move(diag));
    }

    // 6.7.5p2: an alignment specifier (_Alignas) shall not be applied to a
    // typedef (nor a bit-field, function, parameter, or register object).
    if ((!d->specifiers.alignExprs.empty() || !d->specifiers.alignSpec.empty())
        && (d->specifiers.hasStorage(wvmcc::parser::StorageClass::Typedef)
            || d->specifiers.hasStorage(wvmcc::parser::StorageClass::Register)
            || isFunctionDeclarator(d->declarator))) {
        Diagnostic diag;
        diag.severity = Diagnostic::Severity::Error;
        diag.message = "_Alignas may not be applied to a typedef, function, or register object";
        diag.span = d->span;
        diagnostics.push_back(std::move(diag));
    }

    // 6.7.3p2: `restrict` shall qualify a pointer to an object type. A leading
    // `restrict` (in the declaration specifiers, e.g. `restrict int x`) qualifies
    // the base type and is valid only when the declared object is itself a
    // pointer (such as a `restrict`-qualified pointer typedef). Reject it when
    // the object type is not a pointer.
    if (d->specifiers.hasTypeQual(wvmcc::parser::TypeQualifier::Restrict)
        && !isFunctionDeclarator(d->declarator)
        && !d->specifiers.hasStorage(wvmcc::parser::StorageClass::Typedef)) {
        auto ty = canonicalTypeRepr(d->specifiers, d->declarator);
        if (!ty) ty = buildTypeFromDeclaration(d->specifiers, d->declarator, false, nullptr);
        if (ty && ty->kind != TypeNode::Kind::Pointer) {
            Diagnostic diag;
            diag.severity = Diagnostic::Severity::Error;
            diag.message = "restrict requires a pointer type";
            diag.span = d->span;
            diagnostics.push_back(std::move(diag));
        }
    }

    // 6.7.5p3: an alignment specifier's value shall be a valid alignment — a
    // nonnegative integral power of two. `_Alignas(0)` has no effect; anything
    // else that we can evaluate to a non-power-of-two is a constraint violation.
    for (const auto &ae : d->specifiers.alignExprs) {
        if (!ae) continue;
        auto v = ConstExprEvaluator::evalIntegerConstantExpr(ae);
        if (!v.has_value()) continue;                 // unevaluable: don't guess
        long long a = *v;
        if (a < 0 || (a > 0 && (a & (a - 1)) != 0)) {
            Diagnostic diag;
            diag.severity = Diagnostic::Severity::Error;
            diag.message = "requested alignment is not a positive power of two";
            diag.span = d->span;
            diagnostics.push_back(std::move(diag));
        }
    }

    // 6.7.9p3: the type of an initialized object shall be a complete object
    // type (or an array of unknown size). Reject an initializer for an object
    // whose struct/union type has no definition visible at this point.
    if (d->initializer.has_value()) {
        auto objTy = canonicalTypeRepr(d->specifiers, d->declarator);
        if (!objTy) objTy = buildTypeFromDeclaration(d->specifiers, d->declarator, false, nullptr);
        if (objTy && (objTy->kind == TypeNode::Kind::Struct
                      || objTy->kind == TypeNode::Kind::Union)) {
            bool complete = objTy->su && objTy->su->hasBody;
            if (!complete && objTy->su && objTy->su->name.has_value())
                complete = structUnionTagDefs.count(*objTy->su->name) != 0;
            if (!complete) {
                Diagnostic diag;
                diag.severity = Diagnostic::Severity::Error;
                diag.message = "initializer for object of incomplete type";
                diag.span = d->span;
                diagnostics.push_back(std::move(diag));
            }
        }
    }

    // designator indexes must be integer constant expressions regardless of storage class
    if (d->initializer.has_value()) {
        checkDesignatorIndexes(d->initializer.value(), diagnostics);
    }

    // Struct/union specifier semantic check (C 6.7.2.1): if a struct-or-union
    // specifier has a body but contains no named members (neither directly nor
    // via anonymous nested struct/union), the behavior is undefined — report.
    for (const auto &ts : d->specifiers.typeSpecifiers) {
        if (ts.kind == DeclarationSpecifiers::TypeSpecifier::Kind::StructOrUnion) {
            if (ts.su && ts.su->hasBody) {
                if (!structOrUnionHasNamedMember(ts.su)) {
                    Diagnostic diag;
                    diag.severity = Diagnostic::Severity::Error;
                    diag.message = "struct/union has no named members";
                    diag.span = d->span;
                    diagnostics.push_back(std::move(diag));
                }
            }
        } else if (ts.kind == DeclarationSpecifiers::TypeSpecifier::Kind::Atomic) {
            if (ts.atomicInner) {
                for (const auto &its : ts.atomicInner->typeSpecifiers) {
                    if (its.kind == DeclarationSpecifiers::TypeSpecifier::Kind::StructOrUnion) {
                        if (its.su && its.su->hasBody) {
                            if (!structOrUnionHasNamedMember(its.su)) {
                                Diagnostic diag;
                                diag.severity = Diagnostic::Severity::Error;
                                diag.message = "struct/union has no named members";
                                diag.span = d->span;
                                diagnostics.push_back(std::move(diag));
                            }
                        }
                    }
                }
            }
        }
    }

    // Initialization checks (C 6.7.9): basic validation for initializer forms
    if (d->initializer) {
        auto typeNode = canonicalTypeRepr(d->specifiers, d->declarator);
        // helper: count top-level initializer clauses
        auto countInitClauses = [](const InitializerPtr &init)->size_t {
            if (!init) return 0;
            if (init->kind == Initializer::Kind::Expr) return 1;
            return init->clauses.size();
        };
        size_t nclauses = countInitClauses(d->initializer.value());
        // Ensure we have a diagnostics sink usable both from first-pass checks
        // (when `curDiagnostics` may be null) and traversal-time checks.
        auto outDiag = curDiagnostics ? curDiagnostics : &diagnostics;

        // Perform recursive validation of the initializer against the canonical type
        // (this will also complete array sizes in some nested cases).
        validateInitializerAgainstType(typeNode, d->initializer.value(), *outDiag, d->specifiers.hasStorage(StorageClass::Static), *this);

        // Heuristic pre-check: if the specifiers look like a simple scalar
        // type (e.g., `int`) and the declarator is not an array/function,
        // then a braced initializer-list with more than one element is
        // invalid for a scalar. This covers the common `int x = {1,2};`
        // case even if canonicalization later does not yield a Builtin node.
        if (d->initializer.value()->kind == Initializer::Kind::List && nclauses > 1) {
            bool simpleSpec = false;
            if (!d->specifiers.typeSpecifiers.empty() && d->specifiers.typeSpecifiers.front().kind == DeclarationSpecifiers::TypeSpecifier::Kind::Simple) simpleSpec = true;
            if (simpleSpec && !isFunctionDeclarator(d->declarator)) {
                // ensure not an array declarator
                DeclaratorPtr c = d->declarator; bool hasArray = false;
                while (c) { if (c->kind == Declarator::Kind::Array) { hasArray = true; break; } if (c->inner.has_value()) c = c->inner.value(); else break; }
                if (!hasArray) {
                    Diagnostic diag; diag.severity = Diagnostic::Severity::Error;
                    diag.message = "too many initializers for scalar";
                    diag.span = d->span;
                    outDiag->push_back(std::move(diag));
                }
            }
        }

        // If the declaration has an array declarator with no size, try to
        // complete it from the initializer in simple cases (string-literal
        // for char arrays or braced list with no designators).
        DeclaratorPtr arrayDeclWithoutSize = findFirstArrayDeclaratorWithoutSize(d->declarator);
        if (arrayDeclWithoutSize) {
            // string-literal -> char[]/wide-char[] special-case: set size to
            // (element count)+1. The lexer strips the encoding prefix, so the
            // literal's `value` holds one entry per source character regardless
            // of width — element count = value.size(). A wide-char array
            // (wchar_t[]) takes a wide string literal (6.7.9p15); its element is
            // a wider integer Builtin, so accept any integer-Builtin element.
            if (d->initializer.value()->kind == Initializer::Kind::Expr && d->initializer.value()->expr && d->initializer.value()->expr->kind == Expr::Kind::String) {
                if (typeNode && typeNode->kind == TypeNode::Kind::Array && typeNode->element) {
                    bool isCharLike = false;
                    if (typeNode->element->kind == TypeNode::Kind::Builtin) {
                        using S = DeclarationSpecifiers::SimpleTypeSpecifier;
                        for (auto st : typeNode->element->simple) {
                            if (st == S::Char || st == S::Short || st == S::Int
                                || st == S::Long || st == S::Unsigned || st == S::Signed) { isCharLike = true; break; }
                        }
                        if (!isCharLike && typeNode->element->text == "char") isCharLike = true;
                    }
                    if (isCharLike) {
                        auto sl = std::dynamic_pointer_cast<StringLiteral>(d->initializer.value()->expr);
                        if (sl) {
                            long long len = static_cast<long long>(sl->value.size()) + 1; // include NUL
                            arrayDeclWithoutSize->array.size = makeIntegerLiteral(len);
                            if (typeNode && typeNode->kind == TypeNode::Kind::Array) typeNode->sizeExpr = arrayDeclWithoutSize->array.size.value();
                            nclauses = 1;
                        }
                    }
                }
            } else if (d->initializer.value()->kind == Initializer::Kind::List) {
                // Complete the size from the initializer (6.7.9p22): the largest
                // index reached + 1, counting both designated indices and the
                // sequential positions between/after them. A designator `[k]`
                // repositions the cursor to k; the next undesignated clause goes
                // to k+1. This sets the *declarator's* array.size, which is what
                // sizeof/_Static_assert reads — so it must handle designators,
                // not just a plain clause count.
                long long cursor = 0, maxLen = 0;
                for (const auto &cl : d->initializer.value()->clauses) {
                    if (!cl.designators.empty()
                        && cl.designators.front().kind == Designator::Kind::Index
                        && cl.designators.front().index) {
                        auto vi = ConstExprEvaluator::evalIntegerConstantExpr(cl.designators.front().index.value());
                        if (vi.has_value()) cursor = *vi;
                    }
                    cursor += 1;
                    if (cursor > maxLen) maxLen = cursor;
                }
                arrayDeclWithoutSize->array.size = makeIntegerLiteral(maxLen);
                if (typeNode && typeNode->kind == TypeNode::Kind::Array) typeNode->sizeExpr = arrayDeclWithoutSize->array.size.value();
            }
        }

        if (typeNode) {
            if (typeNode->kind == TypeNode::Kind::Builtin) {
                // scalar: initializer list must contain at most one initializer
                if (d->initializer.value()->kind == Initializer::Kind::List && nclauses > 1) {
                            {
                                Diagnostic diag; diag.severity = Diagnostic::Severity::Error;
                                diag.message = "too many initializers for scalar";
                                diag.span = d->span;
                                outDiag->push_back(std::move(diag));
                            }
                }
            } else if (typeNode->kind == TypeNode::Kind::Struct || typeNode->kind == TypeNode::Kind::Union) {
                // count named members
                size_t members = 0;
                if (typeNode->su && typeNode->su->hasBody) {
                    for (const auto &m : typeNode->su->members) {
                        // Count every named declarator: a single struct-declaration
                        // may declare several members (`int a, b;` is two).
                        for (const auto &sd : m.declarators) {
                            if (sd.declarator && !declaratorName(sd.declarator).empty()) members++;
                        }
                    }
                }
                    if (d->initializer.value()->kind == Initializer::Kind::List && members > 0 && nclauses > members) {
                    Diagnostic diag; diag.severity = Diagnostic::Severity::Error;
                    diag.message = "excess elements in initializer";
                    diag.span = d->span;
                    outDiag->push_back(std::move(diag));
                }
                // Basic check for designated member initializers: ensure member exists
                if (d->initializer.value()->kind == Initializer::Kind::List) {
                    for (const auto &cl : d->initializer.value()->clauses) {
                        if (!cl.designators.empty()) {
                            for (const auto &des : cl.designators) {
                                if (des.kind == Designator::Kind::Member) {
                                    bool found = false;
                                    if (typeNode->su && typeNode->su->hasBody) {
                                        for (const auto &m : typeNode->su->members) {
                                            for (const auto &sd : m.declarators) {
                                                if (sd.declarator) {
                                                    if (declaratorName(sd.declarator) == des.member) { found = true; break; }
                                                }
                                            }
                                            if (found) break;
                                        }
                                    }
                                    if (!found) {
                                        Diagnostic diag; diag.severity = Diagnostic::Severity::Error;
                                        diag.message = "designator refers to unknown member '" + des.member + "'";
                                        outDiag->push_back(std::move(diag));
                                    }
                                }
                            }
                        }
                    }
                }
            } else if (typeNode->kind == TypeNode::Kind::Array) {
                // If array size known and initializer is list, check for excess elements
                if (typeNode->sizeExpr.has_value()) {
                    auto v = ConstExprEvaluator::evalIntegerConstantExpr(typeNode->sizeExpr.value());
                    if (v.has_value() && d->initializer.value()->kind == Initializer::Kind::List) {
                        if (nclauses > static_cast<size_t>(*v)) {
                            Diagnostic diag; diag.severity = Diagnostic::Severity::Error;
                            diag.message = "excess elements in initializer";
                            diag.span = d->span;
                            outDiag->push_back(std::move(diag));
                        }
                    }
                }
                // special-case: char array initialized with string literal handled above
            }
        }
    }

    // track internal (static) definitions for duplicate checking
    if (!d->declarator->id.name.empty() && d->specifiers.hasStorage(wvmcc::parser::StorageClass::Static)) {
        std::string nm = d->declarator->id.name;
        bool definitive = d->initializer.has_value();
        auto it = internalDefs.find(nm);
        if (definitive) {
            if (it != internalDefs.end() && it->second.second) {
                // duplicate internal definition handled by parser (constraint checks). Do not emit here.
            }
            internalDefs[nm] = std::make_pair(d->span, true);
        } else {
            if (it == internalDefs.end()) internalDefs[nm] = std::make_pair(d->span, false);
        }
    }

    // Restrict qualifier association check (C 6.7.3.1, conservative):
    // If a restrict-qualified pointer is initialized from the address of an
    // object (e.g., `T * restrict p = &obj;`), record association. If later a
    // non-restrict pointer is initialized to the address of the same object,
    // emit a diagnostic because that may violate the restrict association.
    auto declaratorIsPointer = [](const DeclaratorPtr &dd)->bool {
        DeclaratorPtr cur = dd;
        while (cur) {
            if (cur->kind == Declarator::Kind::Pointer) return true;
            if (cur->inner.has_value()) cur = cur->inner.value(); else break;
        }
        return false;
    };

    bool hasRestrict = false;
    // top-level qualifiers
    if (d->specifiers.hasTypeQual(TypeQualifier::Restrict)) hasRestrict = true;
    // pointer-level qualifiers: check any pointer node
    DeclaratorPtr curd = d->declarator;
    while (curd) {
        if (curd->kind == Declarator::Kind::Pointer) {
            if (hasTypeQual(curd->ptrQual, TypeQualifier::Restrict)) hasRestrict = true;
        }
        if (curd->inner.has_value()) curd = curd->inner.value(); else break;
    }

    if (d->initializer && declaratorIsPointer(d->declarator)) {
        // check initializer is address-of an identifier: unary '&' with IdentifierExpr
        if (d->initializer.value()->kind == Initializer::Kind::Expr && d->initializer.value()->expr) {
            auto ie = d->initializer.value()->expr;
            if (ie->kind == Expr::Kind::Unary) {
                auto ue = std::dynamic_pointer_cast<UnaryExpr>(ie);
                if (ue && ue->op == "&" && ue->rhs && ue->rhs->kind == Expr::Kind::Ident) {
                    auto id = std::dynamic_pointer_cast<IdentifierExpr>(ue->rhs);
                    if (id && !id->name.empty()) {
                        const std::string obj = id->name;
                        // if this declaration has restrict, record association
                        if (hasRestrict) {
                            // record first seen restrict association for this object
                            if (restrictAssoc.find(obj) == restrictAssoc.end()) {
                                restrictAssoc[obj] = std::make_pair(declaratorName(d->declarator), d->span);
                            }
                        } else {
                            // non-restrict pointer initialized to &obj; if we have prior restrict association, emit diagnostic
                            auto it = restrictAssoc.find(obj);
                            if (it != restrictAssoc.end()) {
                                Diagnostic diag;
                                diag.severity = Diagnostic::Severity::Error;
                                diag.message = "non-restrict pointer '" + declaratorName(d->declarator) + "' may alias object '" + obj + "' associated with restrict pointer '" + it->second.first + "'";
                                diag.span = d->span;
                                diagnostics.push_back(std::move(diag));
                            }
                        }
                    }
                }
            }
        }
    }
}

void Semantic::onStaticAssert(const ExternalDecl::StaticAssertPtr &sa) {
    (void)sa;
    if (!sa) return;
    if (!curDiagnostics) return;
    // Evaluate the controlling constant-expression. #81: install a type resolver
    // (backed by typeOfExpr) so `sizeof`/`_Alignof` of a declared object,
    // member, or array element resolves — the standalone evaluator has no symbol
    // table, so these forms are deferred here from the parser.
    ConstExprEvaluator::ResolverScope resolver(
        [this](const ExprPtr &e) -> TypeNodePtr { return typeOfExpr(e).type; },
        // #81: `_Alignof(obj)` reports the object's declared alignment, which
        // _Alignas may raise above the type's natural alignment. File-scope
        // objects record their computed _Alignas value in `seenAlign`; a hit
        // with a value overrides, otherwise (nullopt) the type alignment is used.
        [this](const ExprPtr &e) -> std::optional<long long> {
            if (!e || e->kind != Expr::Kind::Ident) return std::nullopt;
            auto id = std::dynamic_pointer_cast<IdentifierExpr>(e);
            if (!id) return std::nullopt;
            auto it = seenAlign.find(id->name);
            if (it != seenAlign.end() && it->second.value.has_value())
                return it->second.value;
            return std::nullopt;
        });
    auto val = ConstExprEvaluator::evalIntegerConstantExpr(sa->expr);
    if (!val.has_value()) {
        Diagnostic d; d.severity = Diagnostic::Severity::Error; d.message = "_Static_assert requires an integer constant expression";
        if (sa->expr) d.span = sa->expr->span;
        curDiagnostics->push_back(std::move(d));
        return;
    }
    // If value != 0, declaration has no effect; if value == 0, emit diagnostic with string literal text
    if (*val == 0) {
        std::string msgText = "static assertion failed";
        if (sa->message && sa->message->kind == Expr::Kind::String) {
            auto sl = std::dynamic_pointer_cast<StringLiteral>(sa->message);
            if (sl) msgText = sl->value;
        }
        Diagnostic d; d.severity = Diagnostic::Severity::Error; d.message = msgText;
        if (sa->expr) d.span = sa->expr->span;
        curDiagnostics->push_back(std::move(d));
    }
}

// --- Type-classification helpers for expression constraint checks ----------
// These intentionally treat an *unknown* type (null) as "not classifiable" so
// callers stay conservative and never reject an expression whose type we could
// not determine.
namespace {
bool tcIsVoid(const std::shared_ptr<TypeNode> &t) {
    if (!t || t->kind != TypeNode::Kind::Builtin) return false;
    using S = DeclarationSpecifiers::SimpleTypeSpecifier;
    if (t->simple.size() == 1 && t->simple[0] == S::Void) return true;
    if (t->simple.empty() && t->text == "void") return true;
    return false;
}
bool tcIsArithmetic(const std::shared_ptr<TypeNode> &t) {
    if (!t) return false;
    if (t->kind == TypeNode::Kind::Enum) return true;
    if (t->kind != TypeNode::Kind::Builtin) return false;
    if (tcIsVoid(t)) return false;
    // A typedef-name we couldn't resolve (text set, simple empty) is unknown:
    // be conservative and do not classify it as arithmetic so we never reject.
    if (t->simple.empty() && !t->text.empty() && t->text != "void") return false;
    return true;
}
bool tcIsPointer(const std::shared_ptr<TypeNode> &t) {
    return t && t->kind == TypeNode::Kind::Pointer;
}
bool tcIsStructOrUnion(const std::shared_ptr<TypeNode> &t) {
    return t && (t->kind == TypeNode::Kind::Struct || t->kind == TypeNode::Kind::Union);
}
bool tcIsArray(const std::shared_ptr<TypeNode> &t) {
    return t && t->kind == TypeNode::Kind::Array;
}
bool tcIsFunction(const std::shared_ptr<TypeNode> &t) {
    return t && t->kind == TypeNode::Kind::Function;
}
// Scalar = arithmetic or pointer (C 6.2.5p21).
bool tcIsScalar(const std::shared_ptr<TypeNode> &t) {
    return tcIsArithmetic(t) || tcIsPointer(t);
}
// A stable code identifying an arithmetic *builtin* up to type compatibility
// (signedness and rank distinguished; redundant `signed`/`int` spelling
// collapsed so `int` and `signed int` share a code). Returns -1 for anything
// not a classifiable arithmetic builtin, so callers reject only clear
// mismatches and never well-formed code.
int tcArithKind(const std::shared_ptr<TypeNode> &t) {
    if (!t || t->kind != TypeNode::Kind::Builtin) return -1;
    if (t->simple.empty()) return -1;
    using S = DeclarationSpecifiers::SimpleTypeSpecifier;
    bool isUnsigned=false, isSigned=false, isFloat=false, isDouble=false,
         isBool=false, isChar=false, isVoid=false;
    int shortCount=0, longCount=0;
    for (auto s : t->simple) {
        switch (s) {
            case S::Unsigned: isUnsigned=true; break;
            case S::Signed:   isSigned=true; break;
            case S::Float:    isFloat=true; break;
            case S::Double:   isDouble=true; break;
            case S::Bool:     isBool=true; break;
            case S::Char:     isChar=true; break;
            case S::Short:    shortCount++; break;
            case S::Long:     longCount++; break;
            default: break; // Int, etc.
        }
    }
    if (isVoid) return -1;
    if (isBool)   return 1;
    if (isDouble) return 2;          // long double aliases double in wvmcc
    if (isFloat)  return 3;
    if (isChar)   return isUnsigned ? 4 : (isSigned ? 5 : 6); // plain char is its own type
    int rank;
    if (shortCount > 0)      rank = 1;
    else if (longCount >= 2) rank = 4;
    else if (longCount == 1) rank = 3;
    else                     rank = 2; // plain int
    return 10 + rank * 2 + (isUnsigned ? 1 : 0);
}
// True when two pointer types point to *positively* incompatible types — used
// to diagnose ill-formed pointer subtraction / comparison / assignment without
// false positives. Conservative: a `void*` side, an unknown pointee, or any
// non-arithmetic/non-aggregate pointee yields false. Arithmetic pointees are
// incompatible when their tcArithKind differs (int* vs double*); struct/union
// pointees are incompatible when their tags differ.
bool tcPointeesIncompatible(const std::shared_ptr<TypeNode> &a,
                            const std::shared_ptr<TypeNode> &b) {
    if (!tcIsPointer(a) || !tcIsPointer(b)) return false;
    auto pa = a->pointee, pb = b->pointee;
    if (!pa || !pb) return false;
    if (tcIsVoid(pa) || tcIsVoid(pb)) return false;
    int ka = tcArithKind(pa), kb = tcArithKind(pb);
    if (ka >= 0 && kb >= 0) return ka != kb;
    if (tcIsStructOrUnion(pa) && tcIsStructOrUnion(pb))
        return !Semantic::typeNodesEqual(pa, pb);
    return false;
}
} // namespace

Semantic::ExprTypeResult Semantic::typeOfExpr(const ExprPtr &e) const {
    ExprTypeResult res;
    if (!e) return res;
    switch (e->kind) {
        case Expr::Kind::Ident: {
            auto id = std::dynamic_pointer_cast<IdentifierExpr>(e);
            if (!id) break;
            // Block-scope (local) objects take precedence over file-scope names.
            if (const LocalSym *ls = lookupLocal(id->name)) {
                if (ls->type) {
                    res.type = ls->type;
                    if (res.type->kind == TypeNode::Kind::Function) {
                        res.isFunctionDesignator = true;
                    } else {
                        res.isLvalue = true;
                        res.isConst = ls->isConst;
                        res.isRegister = ls->isRegister;
                    }
                }
                break;
            }
            // Look up declared type representation for the identifier (file scope)
            auto it = declaredTypeRepr.find(id->name);
            if (it != declaredTypeRepr.end() && it->second) {
                res.type = it->second;
                if (res.type->kind == TypeNode::Kind::Function) {
                    res.isFunctionDesignator = true;
                } else {
                    // If it's not a function, the identifier designates an object and is an lvalue
                    res.isLvalue = true;
                }
            }
            break;
        }
        case Expr::Kind::Integer: {
            // integer literal -> int type
            auto tn = std::make_shared<TypeNode>();
            tn->kind = TypeNode::Kind::Builtin;
            tn->simple.push_back(DeclarationSpecifiers::SimpleTypeSpecifier::Int);
            res.type = tn;
            break;
        }
        case Expr::Kind::Float: {
            auto tn = std::make_shared<TypeNode>();
            tn->kind = TypeNode::Kind::Builtin;
            tn->simple.push_back(DeclarationSpecifiers::SimpleTypeSpecifier::Double);
            res.type = tn;
            break;
        }
        case Expr::Kind::Char: {
            auto tn = std::make_shared<TypeNode>();
            tn->kind = TypeNode::Kind::Builtin;
            tn->simple.push_back(DeclarationSpecifiers::SimpleTypeSpecifier::Char);
            res.type = tn;
            break;
        }
        case Expr::Kind::String: {
            // string literal is an lvalue of array of char
            auto sl = std::dynamic_pointer_cast<StringLiteral>(e);
            auto elem = std::make_shared<TypeNode>();
            elem->kind = TypeNode::Kind::Builtin;
            elem->simple.push_back(DeclarationSpecifiers::SimpleTypeSpecifier::Char);

            auto arr = std::make_shared<TypeNode>();
            arr->kind = TypeNode::Kind::Array;
            arr->element = elem;
            // sizeExpr left null (unknown/implicit) and array is an lvalue
            res.type = arr;
            res.isLvalue = true;
            break;
        }
        case Expr::Kind::GenericSelection: {
            auto g = std::dynamic_pointer_cast<GenericSelectionExpr>(e);
            if (!g) break;
            // compute controlling expression type (do not evaluate it)
            ExprTypeResult ctrl = typeOfExpr(g->controlling);
            // find first association whose type is compatible
            std::shared_ptr<Expr> chosenExpr;
            for (const auto &assoc : g->assocs) {
                if (!assoc.isDefault && assoc.type) {
                    if (typeNodesEqual(assoc.type, ctrl.type)) { chosenExpr = assoc.expr; break; }
                }
            }
            if (!chosenExpr) {
                // use default association
                for (const auto &assoc : g->assocs) {
                    if (assoc.isDefault) { chosenExpr = assoc.expr; break; }
                }
            }
            if (chosenExpr) {
                // type/value category equals that of the chosen expression
                res = typeOfExpr(chosenExpr);
            }
            break;
        }
        case Expr::Kind::Call: {
            auto c = std::dynamic_pointer_cast<CallExpr>(e);
            if (!c) break;
            // determine callee type
            ExprTypeResult calleeRes = typeOfExpr(c->callee);
            std::shared_ptr<TypeNode> fnType;
            if (calleeRes.isFunctionDesignator && calleeRes.type && calleeRes.type->kind == TypeNode::Kind::Function) {
                fnType = calleeRes.type;
            } else if (calleeRes.type && calleeRes.type->kind == TypeNode::Kind::Pointer && calleeRes.type->pointee && calleeRes.type->pointee->kind == TypeNode::Kind::Function) {
                fnType = calleeRes.type->pointee;
            }
            // compute argument types
            std::vector<ExprTypeResult> argTypes;
            for (const auto &a : c->args) argTypes.push_back(typeOfExpr(a));

            // helper: apply default argument promotions to a TypeNode (returns new node)
            auto applyDefaultPromotions = [&](std::shared_ptr<TypeNode> t)->std::shared_ptr<TypeNode> {
                if (!t) return t;
                if (t->kind != TypeNode::Kind::Builtin) return t;
                using S = DeclarationSpecifiers::SimpleTypeSpecifier;
                // promote char/short -> int; float -> double
                if (!t->simple.empty()) {
                    if (t->simple.size() == 1) {
                        if (t->simple[0] == S::Char || t->simple[0] == S::Short) {
                            auto nt = std::make_shared<TypeNode>(); nt->kind = TypeNode::Kind::Builtin; nt->simple.push_back(S::Int); return nt;
                        }
                        if (t->simple[0] == S::Float) {
                            auto nt = std::make_shared<TypeNode>(); nt->kind = TypeNode::Kind::Builtin; nt->simple.push_back(S::Double); return nt;
                        }
                    }
                }
                return t;
            };

            if (fnType) {
                // Determine result type: function returning object -> return type, otherwise void
                if (fnType->element) res.type = fnType->element;
                else res.isVoid = true;
                // If a prototype is present, check the argument count (C 6.5.2.2p2):
                // the number of arguments shall agree with the number of
                // parameters (more are permitted only for a variadic function).
                if (fnType->hasParamTypeList) {
                    size_t pcount = fnType->params.size();
                    bool countBad = fnType->isVariadic
                        ? (argTypes.size() < pcount)
                        : (argTypes.size() != pcount);
                    if (countBad && curDiagnostics) {
                        Diagnostic d; d.severity = Diagnostic::Severity::Error;
                        d.message = (argTypes.size() > pcount && !fnType->isVariadic)
                            ? "too many arguments to function call"
                            : "argument count does not match function prototype";
                        d.span = e->span; curDiagnostics->push_back(std::move(d));
                    }
                    // NOTE: per-argument *type* compatibility is deliberately not
                    // diagnosed here — modelling the implicit assignment-style
                    // conversions (int<->long, 0->pointer, etc.) accurately is
                    // out of scope and would risk rejecting valid code.
                } else {
                    // no prototype: apply default promotions to arguments (no diagnostic)
                    for (size_t i = 0; i < argTypes.size(); ++i) {
                        if (argTypes[i].type) argTypes[i].type = applyDefaultPromotions(argTypes[i].type);
                    }
                }
            } else {
                // Result type is the function's return type, but we couldn't
                // model the callee type. Only diagnose a non-callable object
                // when we *positively* know the callee is a non-function,
                // non-pointer object (avoid false positives on unmodelled
                // callee expressions whose type we left null).
                res.isVoid = true;
                if (curDiagnostics && calleeRes.type
                    && calleeRes.type->kind != TypeNode::Kind::Pointer
                    && calleeRes.type->kind != TypeNode::Kind::Function
                    && (tcIsArithmetic(calleeRes.type) || tcIsStructOrUnion(calleeRes.type) || tcIsArray(calleeRes.type))) {
                    Diagnostic d; d.severity = Diagnostic::Severity::Error; d.message = "called object is not a function or function pointer"; d.span = c->callee ? c->callee->span : e->span; curDiagnostics->push_back(std::move(d));
                }
            }
            break;
        }
        case Expr::Kind::Member: {
            auto m = std::dynamic_pointer_cast<MemberExpr>(e);
            if (!m) break;
            ExprTypeResult baseRes = typeOfExpr(m->base);
            std::shared_ptr<StructOrUnionSpecifier> suSpec;
            bool baseIsLvalue = baseRes.isLvalue;
            if (m->isArrow) {
                // base must be pointer to struct/union. Only diagnose when we
                // positively know the base is a non-pointer (or a pointer to a
                // non-struct/union); leave unknown types alone.
                if (baseRes.type && baseRes.type->kind != TypeNode::Kind::Pointer) {
                    if (curDiagnostics) {
                        Diagnostic d; d.severity = Diagnostic::Severity::Error; d.message = "left operand of '->' must be pointer to struct/union"; d.span = m->base ? m->base->span : e->span; curDiagnostics->push_back(std::move(d));
                    }
                } else if (baseRes.type && baseRes.type->kind == TypeNode::Kind::Pointer && baseRes.type->pointee
                           && baseRes.type->pointee->kind != TypeNode::Kind::Struct && baseRes.type->pointee->kind != TypeNode::Kind::Union) {
                    if (curDiagnostics) {
                        Diagnostic d; d.severity = Diagnostic::Severity::Error; d.message = "left operand of '->' must point to struct/union type"; d.span = m->base ? m->base->span : e->span; curDiagnostics->push_back(std::move(d));
                    }
                } else if (baseRes.type && baseRes.type->kind == TypeNode::Kind::Pointer && baseRes.type->pointee) {
                    suSpec = baseRes.type->pointee->su;
                    baseIsLvalue = true; // result is lvalue
                }
            } else {
                // '.' operator: base must have struct/union type. Only diagnose
                // when the base type is positively a non-struct/union; leave an
                // unknown (null) base type alone to avoid false positives.
                if (baseRes.type && baseRes.type->kind != TypeNode::Kind::Struct && baseRes.type->kind != TypeNode::Kind::Union) {
                    if (curDiagnostics) {
                        Diagnostic d; d.severity = Diagnostic::Severity::Error; d.message = "left operand of '.' must be struct or union type"; d.span = m->base ? m->base->span : e->span; curDiagnostics->push_back(std::move(d));
                    }
                } else if (baseRes.type) {
                    suSpec = baseRes.type->su;
                }
            }

            if (suSpec) {
                // find member by name
                bool found = false;
                bool anyAnonymous = false; // anonymous (unnamed) member present
                for (const auto &member : suSpec->members) {
                    if (member.declarators.empty()) anyAnonymous = true;
                    for (const auto &sd : member.declarators) {
                        if (!sd.declarator) { anyAnonymous = true; continue; }
                        std::string mname = sd.declarator->id.name;
                        if (mname.empty()) { anyAnonymous = true; continue; }
                        if (mname == m->member) {
                            // build member type from member.specifiers + sd.declarator
                            bool vm = false;
                            auto mt = buildTypeFromDeclaration(member.specifiers, sd.declarator, false, &vm);
                            if (mt) {
                                res.type = mt;
                                res.isLvalue = baseIsLvalue;
                                // 6.5.3.2p1: a bit-field member has no address.
                                res.isBitfield = sd.bitfieldWidth.has_value();
                            }
                            found = true;
                            break;
                        }
                    }
                    if (found) break;
                }
                // Only diagnose a missing member when we have a complete type
                // (a body) and no anonymous members (whose own members would be
                // promoted into this scope and which we don't model here).
                if (!found && curDiagnostics && suSpec->hasBody && !anyAnonymous) {
                    Diagnostic d; d.severity = Diagnostic::Severity::Error; d.message = "no member named '" + m->member + "' in object"; d.span = m->span; curDiagnostics->push_back(std::move(d));
                }
            }
            break;
        }
        case Expr::Kind::PostfixUnary: {
            auto pu = std::dynamic_pointer_cast<PostfixUnaryExpr>(e);
            if (!pu) break;
            ExprTypeResult baseRes = typeOfExpr(pu->base);
            // C 6.5.2.4p1: the operand shall be a modifiable lvalue with
            // arithmetic or pointer type. We only emit a diagnostic when we
            // positively know the operand type is unsuitable (a non-arithmetic,
            // non-pointer object such as a struct/union/array, or a const
            // object). Unknown types are left alone to avoid false positives.
            if (curDiagnostics && baseRes.type) {
                bool okType = tcIsArithmetic(baseRes.type) || tcIsPointer(baseRes.type);
                if (!okType && (tcIsStructOrUnion(baseRes.type) || tcIsArray(baseRes.type) || tcIsVoid(baseRes.type) || tcIsFunction(baseRes.type))) {
                    Diagnostic d; d.severity = Diagnostic::Severity::Error;
                    d.message = "operand of postfix ++/-- must have arithmetic or pointer type";
                    d.span = pu->base ? pu->base->span : e->span;
                    curDiagnostics->push_back(std::move(d));
                } else if (okType && baseRes.isConst) {
                    Diagnostic d; d.severity = Diagnostic::Severity::Error;
                    d.message = "cannot modify a const-qualified object";
                    d.span = pu->base ? pu->base->span : e->span;
                    curDiagnostics->push_back(std::move(d));
                }
            }
            // result has the value of the operand (not an lvalue)
            res.type = baseRes.type;
            res.isLvalue = false;
            break;
        }
        case Expr::Kind::CompoundLiteral: {
            auto cl = std::dynamic_pointer_cast<CompoundLiteral>(e);
            if (!cl) break;
            // The type of a compound literal is the specified type; the result is an lvalue.
            if (cl->type) {
                res.type = cl->type;
                res.isLvalue = true;
                // If it's an array of unknown size, complete the size from the initializer
                if (res.type->kind == TypeNode::Kind::Array && !res.type->sizeExpr.has_value() && cl->init) {
                    if (cl->init->kind == Initializer::Kind::List) {
                        // number of top-level clauses determines array size
                        size_t count = cl->init->clauses.size();
                        auto il = std::make_shared<IntegerLiteral>();
                        il->kind = Expr::Kind::Integer;
                        il->value = (long long)count;
                        il->raw = std::to_string(count);
                        res.type->sizeExpr = il;
                    } else if (cl->init->kind == Initializer::Kind::Expr && cl->init->expr && cl->init->expr->kind == Expr::Kind::String) {
                        // string literal initializes char array: size = strlen + 1
                        auto sl = std::dynamic_pointer_cast<StringLiteral>(cl->init->expr);
                        if (sl) {
                            size_t len = sl->value.size();
                            auto il = std::make_shared<IntegerLiteral>();
                            il->kind = Expr::Kind::Integer;
                            il->value = (long long)(len + 1);
                            il->raw = std::to_string(len + 1);
                            res.type->sizeExpr = il;
                        }
                    }
                }
            }
            break;
        }
        case Expr::Kind::Index: {
            auto ix = std::dynamic_pointer_cast<IndexExpr>(e);
            if (!ix) break;
            ExprTypeResult baseRes = typeOfExpr(ix->base);
            (void)typeOfExpr(ix->index);
            // result is the element/pointee type and is an lvalue
            if (baseRes.type) {
                if (baseRes.type->kind == TypeNode::Kind::Array) {
                    res.type = baseRes.type->element;
                    res.isLvalue = true;
                } else if (baseRes.type->kind == TypeNode::Kind::Pointer) {
                    res.type = baseRes.type->pointee;
                    res.isLvalue = true;
                }
            }
            break;
        }
        case Expr::Kind::Cast: {
            auto cx = std::dynamic_pointer_cast<CastExpr>(e);
            if (!cx) break;
            ExprTypeResult opRes = typeOfExpr(cx->expr); // drive diagnostics on the operand
            // 6.5.4p2-4: a pointer may not be cast to a floating type, nor a
            // floating value to a pointer (only the 6.3.2.3 integer<->pointer
            // conversions are permitted). Reject the pointer<->floating cast.
            if (curDiagnostics && cx->type) {
                bool targetFloat = tcIsArithmetic(cx->type)
                    && (tcArithKind(cx->type) == 2 || tcArithKind(cx->type) == 3);
                bool opFloat = tcIsArithmetic(opRes.type)
                    && (tcArithKind(opRes.type) == 2 || tcArithKind(opRes.type) == 3);
                if ((tcIsPointer(cx->type) && opFloat)
                    || (targetFloat && tcIsPointer(opRes.type))) {
                    Diagnostic d; d.severity = Diagnostic::Severity::Error;
                    d.message = "pointer cannot be cast to or from a floating type";
                    d.span = e->span;
                    curDiagnostics->push_back(std::move(d));
                }
            }
            res.type = cx->type;        // value of the cast has the target type
            res.isLvalue = false;
            break;
        }
        case Expr::Kind::Unary: {
            auto ue = std::dynamic_pointer_cast<UnaryExpr>(e);
            if (!ue) break;
            ExprTypeResult sub = typeOfExpr(ue->rhs);
            if (ue->op == "*") {
                // C 6.5.3.2p2: operand of unary '*' shall be a pointer.
                if (sub.type && tcIsPointer(sub.type)) {
                    res.type = sub.type->pointee;
                    res.isLvalue = true;
                }
                // (a non-pointer operand is already reported by codegen; avoid a
                //  duplicate diagnostic here to keep behavior stable.)
            } else if (ue->op == "&") {
                // 6.5.3.2p1: the operand of unary & shall not designate a
                // bit-field nor a register-declared object.
                if (curDiagnostics && sub.isBitfield) {
                    Diagnostic d; d.severity = Diagnostic::Severity::Error;
                    d.message = "cannot take the address of a bit-field";
                    d.span = ue->rhs ? ue->rhs->span : e->span;
                    curDiagnostics->push_back(std::move(d));
                } else if (curDiagnostics && sub.isRegister) {
                    Diagnostic d; d.severity = Diagnostic::Severity::Error;
                    d.message = "cannot take the address of a register-declared object";
                    d.span = ue->rhs ? ue->rhs->span : e->span;
                    curDiagnostics->push_back(std::move(d));
                }
                // result is a pointer to the operand's type
                auto p = std::make_shared<TypeNode>();
                p->kind = TypeNode::Kind::Pointer;
                p->pointee = sub.type;
                res.type = p;
            } else if (ue->op == "+" || ue->op == "-" || ue->op == "~") {
                res.type = sub.type;
            } else if (ue->op == "!") {
                auto tn = std::make_shared<TypeNode>();
                tn->kind = TypeNode::Kind::Builtin;
                tn->simple.push_back(DeclarationSpecifiers::SimpleTypeSpecifier::Int);
                res.type = tn;
            } else if (ue->op == "++" || ue->op == "--") {
                // 6.5.3.1p1: the operand of prefix ++/-- shall be a modifiable
                // lvalue of arithmetic or pointer type. Mirror the postfix check.
                if (curDiagnostics && sub.type) {
                    bool okType = tcIsArithmetic(sub.type) || tcIsPointer(sub.type);
                    if (!okType && (tcIsStructOrUnion(sub.type) || tcIsArray(sub.type)
                                    || tcIsVoid(sub.type) || tcIsFunction(sub.type))) {
                        Diagnostic d; d.severity = Diagnostic::Severity::Error;
                        d.message = "operand of prefix ++/-- must have arithmetic or pointer type";
                        d.span = ue->rhs ? ue->rhs->span : e->span;
                        curDiagnostics->push_back(std::move(d));
                    } else if (okType && sub.isConst) {
                        Diagnostic d; d.severity = Diagnostic::Severity::Error;
                        d.message = "cannot modify a const-qualified object";
                        d.span = ue->rhs ? ue->rhs->span : e->span;
                        curDiagnostics->push_back(std::move(d));
                    }
                }
                res.type = sub.type;
                res.isLvalue = false;
            } else {
                res.type = sub.type;
            }
            break;
        }
        case Expr::Kind::Ternary: {
            auto te = std::dynamic_pointer_cast<TernaryExpr>(e);
            if (!te) break;
            ExprTypeResult condRes = typeOfExpr(te->cond);
            ExprTypeResult thenRes = typeOfExpr(te->thenExpr);
            ExprTypeResult elseRes = typeOfExpr(te->elseExpr);
            // C 6.5.15p2: the first operand shall have scalar type. Only reject
            // when we positively classify it as a non-scalar (struct/union/
            // array/void); an unknown type is left alone.
            if (curDiagnostics && condRes.type
                && (tcIsStructOrUnion(condRes.type) || tcIsArray(condRes.type) || tcIsVoid(condRes.type))) {
                Diagnostic d; d.severity = Diagnostic::Severity::Error;
                d.message = "first operand of '?:' must have scalar type";
                d.span = te->cond ? te->cond->span : e->span;
                curDiagnostics->push_back(std::move(d));
            }
            // result type: keep it simple — prefer the 'then' branch's type.
            res.type = thenRes.type ? thenRes.type : elseRes.type;
            break;
        }
        case Expr::Kind::Binary: {
            auto be = std::dynamic_pointer_cast<BinaryExpr>(e);
            if (!be) break;
            const std::string &op = be->op;
            ExprTypeResult lhs = typeOfExpr(be->lhs);
            ExprTypeResult rhs = typeOfExpr(be->rhs);

            // Assignment operators (simple and compound). C 6.5.16p2/p3: the
            // left operand shall be a modifiable lvalue.
            bool isAssign = (op == "=" || op == "+=" || op == "-=" || op == "*="
                             || op == "/=" || op == "%=" || op == "<<=" || op == ">>="
                             || op == "&=" || op == "^=" || op == "|=");
            if (isAssign) {
                if (curDiagnostics && lhs.type) {
                    // 6.3.2.1p1: a modifiable lvalue is an lvalue that is not an
                    // array type, not incomplete, and not const-qualified.
                    if (tcIsArray(lhs.type)) {
                        Diagnostic d; d.severity = Diagnostic::Severity::Error;
                        d.message = "array type is not assignable";
                        d.span = be->lhs ? be->lhs->span : e->span;
                        curDiagnostics->push_back(std::move(d));
                    } else if (tcIsFunction(lhs.type)) {
                        Diagnostic d; d.severity = Diagnostic::Severity::Error;
                        d.message = "expression is not assignable";
                        d.span = be->lhs ? be->lhs->span : e->span;
                        curDiagnostics->push_back(std::move(d));
                    } else if (lhs.isConst) {
                        Diagnostic d; d.severity = Diagnostic::Severity::Error;
                        d.message = "cannot assign to a const-qualified object";
                        d.span = be->lhs ? be->lhs->span : e->span;
                        curDiagnostics->push_back(std::move(d));
                    } else if (op == "=" && tcPointeesIncompatible(lhs.type, rhs.type)) {
                        // 6.5.16.1p1: assigning between pointers to incompatible
                        // object types (no void* side, no null constant) needs a
                        // cast.
                        Diagnostic d; d.severity = Diagnostic::Severity::Error;
                        d.message = "assignment to incompatible pointer type";
                        d.span = e->span;
                        curDiagnostics->push_back(std::move(d));
                    } else if (op != "=" && op != "+=" && op != "-=") {
                        // 6.5.16.2p1: every compound operator other than += / -=
                        // requires both operands to have arithmetic type. Reject
                        // a positively non-arithmetic (pointer/struct/array) left
                        // operand.
                        if (tcIsPointer(lhs.type) || tcIsStructOrUnion(lhs.type)
                            || tcIsArray(lhs.type)) {
                            Diagnostic d; d.severity = Diagnostic::Severity::Error;
                            d.message = "operand of '" + op + "' must have arithmetic type";
                            d.span = be->lhs ? be->lhs->span : e->span;
                            curDiagnostics->push_back(std::move(d));
                        }
                    } else if ((op == "+=" || op == "-=") && tcIsPointer(lhs.type)
                               && rhs.type && !tcIsArithmetic(rhs.type)) {
                        // 6.5.16.2p1: pointer += / -= requires an integer right
                        // operand. (Only flag a positively non-arithmetic rhs.)
                        if (tcIsPointer(rhs.type) || tcIsStructOrUnion(rhs.type)
                            || tcIsArray(rhs.type)) {
                            Diagnostic d; d.severity = Diagnostic::Severity::Error;
                            d.message = "operand of '" + op + "' must have integer type";
                            d.span = be->rhs ? be->rhs->span : e->span;
                            curDiagnostics->push_back(std::move(d));
                        }
                    }
                }
                // result type/value category: the type of the left operand
                // (after lvalue conversion); not an lvalue.
                res.type = lhs.type;
                res.isLvalue = false;
                break;
            }

            // 6.5.6p2: a pointer operand of an additive operator shall point to a
            // *complete* object type. Flag a pointer to a never-defined named
            // struct/union tag (conservative: named tags only; void* arithmetic
            // is a common extension we don't reject).
            auto pointeeIncomplete = [this](const std::shared_ptr<TypeNode> &t) -> bool {
                if (!t || t->kind != TypeNode::Kind::Pointer || !t->pointee) return false;
                const auto &pe = t->pointee;
                if ((pe->kind == TypeNode::Kind::Struct || pe->kind == TypeNode::Kind::Union)
                    && pe->su && pe->su->name.has_value())
                    return !(pe->su->hasBody
                             || structUnionTagDefs.count(*pe->su->name) != 0);
                return false;
            };
            // Additive operators: C 6.5.6p2 — for '+', at most one operand may
            // be a pointer; adding two pointers is a constraint violation.
            if (op == "+") {
                if (curDiagnostics && tcIsPointer(lhs.type) && tcIsPointer(rhs.type)) {
                    Diagnostic d; d.severity = Diagnostic::Severity::Error;
                    d.message = "invalid operands to binary '+' (two pointers)";
                    d.span = e->span;
                    curDiagnostics->push_back(std::move(d));
                } else if (curDiagnostics
                           && (pointeeIncomplete(lhs.type) || pointeeIncomplete(rhs.type))) {
                    Diagnostic d; d.severity = Diagnostic::Severity::Error;
                    d.message = "arithmetic on a pointer to an incomplete type";
                    d.span = e->span;
                    curDiagnostics->push_back(std::move(d));
                }
                // result type heuristic
                if (tcIsPointer(lhs.type)) res.type = lhs.type;
                else if (tcIsPointer(rhs.type)) res.type = rhs.type;
                else res.type = lhs.type ? lhs.type : rhs.type;
                break;
            }
            // Subtraction and other binary ops: just propagate a best-effort
            // result type without new diagnostics.
            if (op == "-") {
                if (tcIsPointer(lhs.type) && tcIsPointer(rhs.type)) {
                    // 6.5.6p3: both operands must point to compatible object
                    // types; subtracting int* and double* is a constraint
                    // violation.
                    if (curDiagnostics && tcPointeesIncompatible(lhs.type, rhs.type)) {
                        Diagnostic d; d.severity = Diagnostic::Severity::Error;
                        d.message = "subtraction of pointers to incompatible types";
                        d.span = e->span;
                        curDiagnostics->push_back(std::move(d));
                    }
                    // pointer difference -> integer (ptrdiff_t); leave as int-ish
                    auto tn = std::make_shared<TypeNode>();
                    tn->kind = TypeNode::Kind::Builtin;
                    tn->simple.push_back(DeclarationSpecifiers::SimpleTypeSpecifier::Long);
                    res.type = tn;
                } else if (tcIsPointer(lhs.type)) {
                    if (curDiagnostics && pointeeIncomplete(lhs.type)) {
                        Diagnostic d; d.severity = Diagnostic::Severity::Error;
                        d.message = "arithmetic on a pointer to an incomplete type";
                        d.span = e->span;
                        curDiagnostics->push_back(std::move(d));
                    }
                    res.type = lhs.type;
                } else {
                    res.type = lhs.type ? lhs.type : rhs.type;
                }
                break;
            }
            // Relational/equality/logical -> int result.
            if (op == "<" || op == ">" || op == "<=" || op == ">=" || op == "=="
                || op == "!=" || op == "&&" || op == "||") {
                if (curDiagnostics) {
                    if (op == "&&" || op == "||") {
                        // 6.5.13p2 / 6.5.14p2: both operands shall be scalar.
                        // Reject a positively non-scalar (struct/union/array/void)
                        // operand.
                        auto nonScalar = [](const std::shared_ptr<TypeNode> &t) {
                            return tcIsStructOrUnion(t) || tcIsArray(t) || tcIsVoid(t);
                        };
                        if (nonScalar(lhs.type) || nonScalar(rhs.type)) {
                            Diagnostic d; d.severity = Diagnostic::Severity::Error;
                            d.message = "operand of '" + op + "' must have scalar type";
                            d.span = e->span;
                            curDiagnostics->push_back(std::move(d));
                        }
                    } else if (tcPointeesIncompatible(lhs.type, rhs.type)) {
                        // 6.5.8p2 (relational) / 6.5.9p2 (equality): comparing
                        // pointers to incompatible object types (no void* side,
                        // no null constant) is a constraint violation.
                        Diagnostic d; d.severity = Diagnostic::Severity::Error;
                        d.message = "comparison of pointers to incompatible types";
                        d.span = e->span;
                        curDiagnostics->push_back(std::move(d));
                    }
                }
                auto tn = std::make_shared<TypeNode>();
                tn->kind = TypeNode::Kind::Builtin;
                tn->simple.push_back(DeclarationSpecifiers::SimpleTypeSpecifier::Int);
                res.type = tn;
                break;
            }
            // Comma operator: type/value of the right operand.
            if (op == ",") {
                res.type = rhs.type;
                res.isLvalue = rhs.isLvalue;
                res.isConst = rhs.isConst;
                break;
            }
            // Remaining arithmetic/bitwise/shift operators: best-effort type.
            res.type = lhs.type ? lhs.type : rhs.type;
            break;
        }
        case Expr::Kind::Sizeof: {
            auto so = std::dynamic_pointer_cast<SizeofExpr>(e);
            if (!so) break;
            // sizeof yields size_t; we report it as an (unsigned long) builtin.
            auto resultTy = std::make_shared<TypeNode>();
            resultTy->kind = TypeNode::Kind::Builtin;
            resultTy->simple.push_back(DeclarationSpecifiers::SimpleTypeSpecifier::Unsigned);
            resultTy->simple.push_back(DeclarationSpecifiers::SimpleTypeSpecifier::Long);
            res.type = resultTy;

            // sizeof(type-name) form: check the operand type for completeness.
            if (so->typeSpecs.has_value()) {
                // Build the operand type from the recorded specifiers (no
                // declarator: this is the bare `sizeof(T)` type-name form).
                auto opType = canonicalTypeRepr(*so->typeSpecs, nullptr);
                if (!opType) opType = buildTypeFromDeclaration(*so->typeSpecs, nullptr, false, nullptr);
                if (curDiagnostics && tcIsVoid(opType)) {
                    Diagnostic d; d.severity = Diagnostic::Severity::Error;
                    d.message = "invalid application of 'sizeof' to an incomplete type 'void'";
                    d.span = e->span;
                    curDiagnostics->push_back(std::move(d));
                }
                break;
            }
            // sizeof expression form: the operand is an unevaluated expression.
            if (so->expr) {
                ExprTypeResult opRes = typeOfExpr(so->expr);
                // C 6.5.3.4p1: sizeof shall not be applied to a function type.
                if (curDiagnostics && (opRes.isFunctionDesignator || tcIsFunction(opRes.type))) {
                    Diagnostic d; d.severity = Diagnostic::Severity::Error;
                    d.message = "invalid application of 'sizeof' to a function type";
                    d.span = so->expr->span;
                    curDiagnostics->push_back(std::move(d));
                } else if (curDiagnostics && tcIsVoid(opRes.type)) {
                    Diagnostic d; d.severity = Diagnostic::Severity::Error;
                    d.message = "invalid application of 'sizeof' to an incomplete type 'void'";
                    d.span = so->expr->span;
                    curDiagnostics->push_back(std::move(d));
                }
            }
            break;
        }
        default: {
            // For other expression kinds, leave type empty for now.
            break;
        }
    }
    return res;
}

void Semantic::checkFunction(const FunctionDefPtr &f, std::vector<wvmcc::Diagnostic> &diagnostics) {
    if (!f) return;
    if (!f->declarator) {
        Diagnostic diag;
        diag.severity = Diagnostic::Severity::Error;
        diag.message = "function without declarator";
        diag.span = f->span;
        diagnostics.push_back(std::move(diag));
        return;
    }
    std::string name = declaratorName(f->declarator);
    if (name.empty()) {
        Diagnostic diag;
        diag.severity = Diagnostic::Severity::Error;
        diag.message = "function with empty name";
        diag.span = f->declarator->span;
        diagnostics.push_back(std::move(diag));
    }

    // Track internal (static) function definitions for duplicate checking
    if (f->specifiers.hasStorage(wvmcc::parser::StorageClass::Static)) {
        std::string nm = f->declarator->id.name;
        auto it = internalDefs.find(nm);
        if (it != internalDefs.end() && it->second.second) {
            // duplicate internal definition handled by parser (constraint checks). Do not emit here.
        }
        internalDefs[nm] = std::make_pair(f->span, true);
    }

    // Basic parameter checks (presence / empty identifier)
    for (const auto &p : f->params) {
        if (p.declarator && p.declarator->id.name.empty()) {
            if (verbose_) {
                Diagnostic diag;
                diag.severity = Diagnostic::Severity::Warning;
                diag.message = "parameter with empty identifier in function";
                diag.span = p.declarator->span;
                diagnostics.push_back(std::move(diag));
            }
        }
    }

    // If this is a function with a body, we also need to check block-scope
    // declarations (these are handled during traversal via onDeclaration and
    // functionDepth tracking).
}

} // namespace wvmcc::parser
