#include "Semantic.hpp"

#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <string>
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
// Helper: determine whether an initializer is a constant (or composed of constants).
static bool initializerIsConstant(const InitializerPtr &init, std::vector<wvmcc::Diagnostic> &diagnostics) {
    if (!init) return false;
    if (init->kind == Initializer::Kind::Expr) {
        if (!init->expr) return false;
        if (init->expr->kind == Expr::Kind::String) return true;
        return ConstExprEvaluator::isIntegerConstantExpr(init->expr);
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

        // If no size and no designators, size can be completed from count
        if (!type->sizeExpr.has_value() && !anyDesignators) {
            type->sizeExpr = makeIntegerLiteral(static_cast<long long>(nclauses));
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
            for (const auto &p : layer->function.params) {
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
            // check if qualifiers-only differ
            if (stripQualParts(prev) == stripQualParts(sig)) {
                Diagnostic diag;
                diag.severity = Diagnostic::Severity::Error;
                diag.message = "incompatible declaration for '" + name + "': qualifiers differ" + (prevLine>0 ? (" (previous at line " + std::to_string(prevLine) + ")") : std::string());
                diag.span = f->declarator->span;
                curDiagnostics->push_back(std::move(diag));
            } else {
                        // fallback: if structural type representations differ, report incompatible declaration
                        auto itype = declaredTypeRepr.find(name);
                        if (itype != declaredTypeRepr.end() && !Semantic::typeNodesEqual(itype->second, canonRepr)) {
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
            if (stripQualParts(prev) == stripQualParts(sig)) {
                Diagnostic diag;
                diag.severity = Diagnostic::Severity::Error;
                diag.message = "incompatible declaration for '" + name + "': qualifiers differ" + (prevLine>0 ? (" (previous at line " + std::to_string(prevLine) + ")") : std::string());
                diag.span = d->declarator->span;
                curDiagnostics->push_back(std::move(diag));
                } else {
                    auto itype = declaredTypeRepr.find(name);
                    if (itype != declaredTypeRepr.end() && !Semantic::typeNodesEqual(itype->second, canonRepr)) {
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
            // Compute canonical alignment and numeric value (if possible)
            auto [maybeVal, canon] = computeAlignFromSpecsTU(d->specifiers);
            std::string alignStr = canon;
            // record definition marker
            if (isDef && !d->specifiers.hasStorage(wvmcc::parser::StorageClass::Static)) {
                recordDef(rname, d->declarator->span);
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
    // external/internal linkage with an initializer, report error.
    if (functionDepth > 0 && curDiagnostics) {
        if (d->initializer.has_value() && (d->specifiers.hasStorage(wvmcc::parser::StorageClass::Extern) || d->specifiers.hasStorage(wvmcc::parser::StorageClass::Static))) {
            Diagnostic diag;
            diag.severity = Diagnostic::Severity::Error;
            diag.message = "declaration at block scope with external/internal linkage shall not have an initializer";
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

void Semantic::onEnterFunction(const FunctionDefPtr &f) {
    (void)f;
    functionDepth++;
}

void Semantic::onExitFunction(const FunctionDefPtr &f) {
    (void)f;
    if (functionDepth > 0) --functionDepth;
}

bool Semantic::run(std::vector<wvmcc::Diagnostic> &diagnostics) {
    if (!tu_) return true;
    // First pass: per-external checks (tags/enums, storage-class constraints,
    // collect internal (static) definitions for duplicate checking)
    internalDefs.clear();
    for (auto &ext : tu_->externals) {
        checkExternal(ext, diagnostics);
    }

    // clear any previous state for external def/use collection
    defCount.clear();
    firstDefSpan.clear();
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

    // Warn about identifier uses with no external definition in TU
    for (const auto &name : usedNames) {
        auto it = defCount.find(name);
        int count = (it == defCount.end()) ? 0 : it->second;
        if (count == 0) {
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
        checkFunction(f, diagnostics);
    } else if (std::holds_alternative<DeclarationPtr>(e->decl)) {
        auto d = std::get<DeclarationPtr>(e->decl);
        // inspect declaration specifiers for tag definitions
        processTypeSpecifiersForTags(d->specifiers, structUnionTagDefs, enumTagDefs, d->span, diagnostics, seenSuDefs_);
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

void Semantic::checkDeclaration(const DeclarationPtr &d, std::vector<wvmcc::Diagnostic> &diagnostics) {
    if (!d) return;
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
    // External-level semantic checks: storage-class constraints
    if (d->specifiers.hasStorage(wvmcc::parser::StorageClass::Auto) || d->specifiers.hasStorage(wvmcc::parser::StorageClass::Register)) {
        Diagnostic diag;
        diag.severity = Diagnostic::Severity::Error;
        if (d->specifiers.hasStorage(wvmcc::parser::StorageClass::Auto)) diag.message = "storage-class specifier 'auto' is not allowed in external declarations";
        else diag.message = "storage-class specifier 'register' is not allowed in external declarations";
        diag.span = d->declarator->span;
        diagnostics.push_back(std::move(diag));
    }

    // static storage duration initializers must be constant at external scope
    if (d->initializer.has_value() && d->specifiers.hasStorage(wvmcc::parser::StorageClass::Static)) {
        if (!initializerIsConstant(d->initializer.value(), diagnostics)) {
            Diagnostic diag;
            diag.severity = Diagnostic::Severity::Error;
            diag.message = "initializer for object with static storage duration must be constant expression or string literal";
            diag.span = d->span;
            diagnostics.push_back(std::move(diag));
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
            // string-literal -> char[] special-case: set size to length+1
            if (d->initializer.value()->kind == Initializer::Kind::Expr && d->initializer.value()->expr && d->initializer.value()->expr->kind == Expr::Kind::String) {
                if (typeNode && typeNode->kind == TypeNode::Kind::Array && typeNode->element) {
                    bool isChar = false;
                    if (typeNode->element->kind == TypeNode::Kind::Builtin) {
                        for (auto st : typeNode->element->simple) {
                            using S = DeclarationSpecifiers::SimpleTypeSpecifier;
                            if (st == S::Char) { isChar = true; break; }
                        }
                        if (!isChar && !typeNode->element->text.empty()) {
                            if (typeNode->element->text == "char") isChar = true;
                        }
                    }
                    if (isChar) {
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
                bool anyDesignators = false;
                for (const auto &cl : d->initializer.value()->clauses) { if (!cl.designators.empty()) { anyDesignators = true; break; } }
                if (!anyDesignators) {
                    long long sz = static_cast<long long>(nclauses);
                    arrayDeclWithoutSize->array.size = makeIntegerLiteral(sz);
                    if (typeNode && typeNode->kind == TypeNode::Kind::Array) typeNode->sizeExpr = arrayDeclWithoutSize->array.size.value();
                }
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
                        for (const auto &sd : m.declarators) {
                            if (sd.declarator) {
                                if (!declaratorName(sd.declarator).empty()) { members++; break; }
                            }
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
    // Evaluate the controlling constant-expression
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

Semantic::ExprTypeResult Semantic::typeOfExpr(const ExprPtr &e) const {
    ExprTypeResult res;
    if (!e) return res;
    switch (e->kind) {
        case Expr::Kind::Ident: {
            auto id = std::dynamic_pointer_cast<IdentifierExpr>(e);
            if (!id) break;
            // Look up declared type representation for the identifier
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
                // If prototype present, check argument count and types
                if (fnType->hasParamTypeList) {
                    size_t pcount = fnType->params.size();
                    if (argTypes.size() != pcount && curDiagnostics) {
                        Diagnostic d; d.severity = Diagnostic::Severity::Error; d.message = "argument count does not match function prototype";
                        d.span = e->span; curDiagnostics->push_back(std::move(d));
                    }
                    // compare each parameter where possible
                    size_t n = std::min(argTypes.size(), pcount);
                    for (size_t i = 0; i < n; ++i) {
                        auto at = argTypes[i].type;
                        auto pt = fnType->params[i];
                        if (!typeNodesEqual(at, pt) && curDiagnostics) {
                            Diagnostic d; d.severity = Diagnostic::Severity::Error; d.message = "argument type mismatch for parameter " + std::to_string(i);
                            if (c->args.size() > i && c->args[i]) d.span = c->args[i]->span;
                            curDiagnostics->push_back(std::move(d));
                        }
                    }
                } else {
                    // no prototype: apply default promotions to arguments (no diagnostic)
                    for (size_t i = 0; i < argTypes.size(); ++i) {
                        if (argTypes[i].type) argTypes[i].type = applyDefaultPromotions(argTypes[i].type);
                    }
                }
            } else {
                // callee is not function or pointer to function: call has void type per C
                res.isVoid = true;
                if (curDiagnostics) {
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
                // base must be pointer to struct/union
                if (!baseRes.type || baseRes.type->kind != TypeNode::Kind::Pointer || !baseRes.type->pointee) {
                    if (curDiagnostics) {
                        Diagnostic d; d.severity = Diagnostic::Severity::Error; d.message = "left operand of '->' must be pointer to struct/union"; d.span = m->base ? m->base->span : e->span; curDiagnostics->push_back(std::move(d));
                    }
                } else if (baseRes.type->pointee->kind != TypeNode::Kind::Struct && baseRes.type->pointee->kind != TypeNode::Kind::Union) {
                    if (curDiagnostics) {
                        Diagnostic d; d.severity = Diagnostic::Severity::Error; d.message = "left operand of '->' must point to struct/union type"; d.span = m->base ? m->base->span : e->span; curDiagnostics->push_back(std::move(d));
                    }
                } else {
                    suSpec = baseRes.type->pointee->su;
                    baseIsLvalue = true; // result is lvalue
                }
            } else {
                // '.' operator: base must be lvalue of struct/union
                if (!baseRes.type || (baseRes.type->kind != TypeNode::Kind::Struct && baseRes.type->kind != TypeNode::Kind::Union)) {
                    if (curDiagnostics) {
                        Diagnostic d; d.severity = Diagnostic::Severity::Error; d.message = "left operand of '.' must be struct or union type"; d.span = m->base ? m->base->span : e->span; curDiagnostics->push_back(std::move(d));
                    }
                } else {
                    suSpec = baseRes.type->su;
                }
            }

            if (suSpec) {
                // find member by name
                bool found = false;
                for (const auto &member : suSpec->members) {
                    for (const auto &sd : member.declarators) {
                        if (!sd.declarator) continue;
                        std::string mname = sd.declarator->id.name;
                        if (mname == m->member) {
                            // build member type from member.specifiers + sd.declarator
                            bool vm = false;
                            auto mt = buildTypeFromDeclaration(member.specifiers, sd.declarator, false, &vm);
                            if (mt) {
                                res.type = mt;
                                res.isLvalue = baseIsLvalue;
                            }
                            found = true;
                            break;
                        }
                    }
                    if (found) break;
                }
                if (!found && curDiagnostics) {
                    Diagnostic d; d.severity = Diagnostic::Severity::Error; d.message = "no member named '" + m->member + "' in object"; d.span = m->span; curDiagnostics->push_back(std::move(d));
                }
            }
            break;
        }
        case Expr::Kind::PostfixUnary: {
            auto pu = std::dynamic_pointer_cast<PostfixUnaryExpr>(e);
            if (!pu) break;
            ExprTypeResult baseRes = typeOfExpr(pu->base);
            // operand must be modifiable lvalue
            if (!baseRes.isLvalue && curDiagnostics) {
                Diagnostic d; d.severity = Diagnostic::Severity::Error;
                d.message = "operand of postfix operator must be modifiable lvalue";
                d.span = pu->base ? pu->base->span : e->span;
                curDiagnostics->push_back(std::move(d));
            }
            // operand type must be arithmetic or pointer
            bool okType = false;
            if (baseRes.type) {
                if (baseRes.type->kind == TypeNode::Kind::Builtin) okType = true;
                if (baseRes.type->kind == TypeNode::Kind::Pointer) okType = true;
                if (baseRes.type->kind == TypeNode::Kind::Enum) okType = true;
            }
            if (!okType && curDiagnostics) {
                Diagnostic d; d.severity = Diagnostic::Severity::Error;
                d.message = "operand of postfix ++/-- must have arithmetic or pointer type";
                d.span = pu->base ? pu->base->span : e->span;
                curDiagnostics->push_back(std::move(d));
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
