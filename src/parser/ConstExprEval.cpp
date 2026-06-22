#include "ConstExprEval.hpp"
#include "AST.hpp"
#include <cstdint>

namespace wvmcc::parser {

namespace {

// Internal evaluation result that, unlike the public `optional<long long>`,
// tracks whether the value has unsigned type. This lets the relational /
// division / shift branches apply the correct (signed vs unsigned) semantics
// required by C 6.4.4.1p5 and the usual arithmetic conversions, e.g.
// `18446744073709551615ULL > 0` must be true (UINT64_MAX, not -1).
struct ICEValue {
    long long v{0};
    bool isUnsigned{false};
};

using STS = DeclarationSpecifiers::SimpleTypeSpecifier;

// #81: the active type resolver (null at parser time; set by the semantic pass
// via ResolverScope so `sizeof obj` can resolve the object's type). Single-
// threaded compiler, so a plain file-scope pointer suffices.
static ConstExprEvaluator::TypeResolver *g_typeResolver = nullptr;

// Forward declarations for the mutually-recursive size/align helpers.
static long long suByteSize(const std::shared_ptr<StructOrUnionSpecifier> &su, bool wantAlign);
static long long typeNodeSize(const TypeNodePtr &t, bool wantAlign);

// Forward declarations for the mutually-recursive constant evaluators.
static std::optional<ICEValue> evalICE(const ExprPtr &e);
// Fold a floating-point constant expression to a double, or nullopt if it is
// not a constant arithmetic expression. Used for the relational/equality
// folding relaxation (see evalICE's Binary case).
static std::optional<double> foldDouble(const ExprPtr &e);

// LP64 scalar sizing, mirroring codegen/TypeMap (kept self-contained so the
// parser layer does not depend on codegen). `wantAlign` selects alignment
// (which equals the size for these scalar types) over byte size.
static long long simpleSize(const std::vector<STS> &simple, bool /*wantAlign*/) {
    if (simple.empty()) return 4; // bare `int`
    bool hasDouble = false, hasFloat = false, hasLong = false,
         hasShort = false, hasChar = false, hasBool = false, hasVoid = false;
    for (auto s : simple) {
        if (s == STS::Double) hasDouble = true;
        else if (s == STS::Float) hasFloat = true;
        else if (s == STS::Long)  hasLong  = true;
        else if (s == STS::Short) hasShort = true;
        else if (s == STS::Char)  hasChar  = true;
        else if (s == STS::Bool)  hasBool  = true;
        else if (s == STS::Void)  hasVoid  = true;
    }
    if (hasDouble) return 8; // long double → double in wvmcc
    if (hasFloat)  return 4;
    if (hasLong)   return 8;
    if (hasShort)  return 2;
    if (hasChar || hasBool) return 1;
    if (hasVoid)   return 0;
    return 4; // int / signed / unsigned / enum-ish
}

// Compute size (or alignment) of a struct/union from its parsed body. Sequential
// layout with natural alignment, mirroring codegen/LayoutEngine for the common
// (non-bit-field) case. Returns -1 if the type is incomplete or contains a
// member whose size we cannot determine here (so sizeof stays a non-ICE).
static long long suByteSize(const std::shared_ptr<StructOrUnionSpecifier> &su, bool wantAlign) {
    if (!su || !su->hasBody) return -1;
    const bool isUnion = (su->kind == StructOrUnionSpecifier::Kind::Union);
    long long offset = 0;     // running offset for struct layout
    long long maxSize = 0;    // largest member (union)
    long long maxAlign = 1;   // overall alignment
    for (const auto &m : su->members) {
        // Determine the member's base type size/alignment from its specifiers.
        long long baseSize = -1, baseAlign = -1;
        // Bit-fields are not modelled here; their presence makes us bail.
        bool sawBitfield = false;
        for (const auto &sd : m.declarators) if (sd.bitfieldWidth.has_value()) sawBitfield = true;

        // Resolve the declaration's base type.
        TypeNodePtr baseType;
        {
            // Build a lightweight TypeNode from the member's first type specifier.
            for (const auto &ts : m.specifiers.typeSpecifiers) {
                if (ts.kind == DeclarationSpecifiers::TypeSpecifier::Kind::Simple) {
                    auto tn = std::make_shared<TypeNode>();
                    tn->kind = TypeNode::Kind::Builtin;
                    tn->simple = ts.simple;
                    baseType = tn;
                    break;
                } else if (ts.kind == DeclarationSpecifiers::TypeSpecifier::Kind::StructOrUnion && ts.su) {
                    auto tn = std::make_shared<TypeNode>();
                    tn->kind = (ts.su->kind == StructOrUnionSpecifier::Kind::Struct)
                                   ? TypeNode::Kind::Struct : TypeNode::Kind::Union;
                    tn->su = ts.su;
                    baseType = tn;
                    break;
                } else if (ts.kind == DeclarationSpecifiers::TypeSpecifier::Kind::Enum) {
                    auto tn = std::make_shared<TypeNode>();
                    tn->kind = TypeNode::Kind::Enum;
                    baseType = tn;
                    break;
                } else {
                    // typedef-name / atomic / other: cannot size self-contained.
                    return -1;
                }
            }
        }
        if (!baseType) return -1;

        // For each declarator, account for pointer/array adornments by walking
        // the declarator chain and wrapping the base TypeNode accordingly.
        auto memberSizeAlign = [&](const StructDeclarator &sd, long long &sz, long long &al) -> bool {
            TypeNodePtr ty = baseType;
            DeclaratorPtr d = sd.declarator;
            bool sawArray = false, sawArrayUnsized = false;
            // Walk outermost→inner; for our purposes pointer/array adornments at
            // any level affect the size. A function declarator makes the member
            // un-sizeable (members can't be functions, but bail defensively).
            while (d) {
                if (d->kind == Declarator::Kind::Pointer) {
                    auto p = std::make_shared<TypeNode>();
                    p->kind = TypeNode::Kind::Pointer;
                    p->pointee = ty;
                    ty = p;
                } else if (d->kind == Declarator::Kind::Array) {
                    auto a = std::make_shared<TypeNode>();
                    a->kind = TypeNode::Kind::Array;
                    a->element = ty;
                    if (d->array.size.has_value()) a->sizeExpr = d->array.size;
                    else sawArrayUnsized = true;
                    sawArray = true;
                    ty = a;
                } else if (d->kind == Declarator::Kind::Function) {
                    return false; // not sizeable here
                }
                if (d->inner.has_value()) d = *d->inner; else break;
            }
            (void)sawArray;
            if (sawArrayUnsized) return false; // flexible array member: not sizeable
            sz = typeNodeSize(ty, false);
            al = typeNodeSize(ty, true);
            return sz >= 0 && al >= 0;
        };

        if (sawBitfield) return -1;

        if (m.declarators.empty()) {
            // Anonymous member (e.g. anonymous struct/union): use base type.
            baseSize = typeNodeSize(baseType, false);
            baseAlign = typeNodeSize(baseType, true);
            if (baseSize < 0 || baseAlign < 0) return -1;
            if (baseAlign > maxAlign) maxAlign = baseAlign;
            if (isUnion) { if (baseSize > maxSize) maxSize = baseSize; }
            else {
                long long pad = (offset % baseAlign == 0) ? 0 : (baseAlign - offset % baseAlign);
                offset += pad + baseSize;
            }
            continue;
        }

        for (const auto &sd : m.declarators) {
            long long sz = -1, al = -1;
            if (!memberSizeAlign(sd, sz, al)) return -1;
            if (al > maxAlign) maxAlign = al;
            if (isUnion) { if (sz > maxSize) maxSize = sz; }
            else {
                long long pad = (al > 0 && offset % al != 0) ? (al - offset % al) : 0;
                offset += pad + sz;
            }
        }
        (void)baseSize; (void)baseAlign;
    }
    if (wantAlign) return maxAlign;
    long long total = isUnion ? maxSize : offset;
    // Pad the overall struct/union up to its alignment.
    if (maxAlign > 0 && total % maxAlign != 0) total += maxAlign - total % maxAlign;
    if (total == 0) total = isUnion ? 0 : 0; // empty aggregate
    return total;
}

// Size (or alignment) of a TypeNode in the LP64 model. Returns -1 when the size
// cannot be determined as a constant (incomplete type, VLA, typedef-name we
// cannot resolve here), which keeps such `sizeof`/`_Alignof` out of ICEs.
static long long typeNodeSize(const TypeNodePtr &t, bool wantAlign) {
    if (!t) return -1;
    switch (t->kind) {
        case TypeNode::Kind::Builtin: {
            if (t->simple.empty()) {
                // Textual / typedef fallback — not resolvable self-contained.
                if (!t->text.empty()) return -1;
                return 4;
            }
            long long s = simpleSize(t->simple, wantAlign);
            if (wantAlign) return s > 0 ? s : 1;
            return s;
        }
        case TypeNode::Kind::Pointer:
            return 8; // wasm64: both size and alignment are 8
        case TypeNode::Kind::Array: {
            if (!t->element) return -1;
            long long elem = typeNodeSize(t->element, wantAlign);
            if (elem < 0) return -1;
            if (wantAlign) return elem;
            if (t->sizeExpr) {
                auto n = ConstExprEvaluator::evalIntegerConstantExpr(*t->sizeExpr);
                if (n.has_value() && *n >= 0) return elem * (long long)*n;
                return -1; // VLA / unknown bound: not a constant size
            }
            return -1; // incomplete array
        }
        case TypeNode::Kind::Struct:
        case TypeNode::Kind::Union:
            return suByteSize(t->su, wantAlign);
        case TypeNode::Kind::Enum:
            return 4;
        case TypeNode::Kind::Qualified:
            return typeNodeSize(t->pointee, wantAlign);
        case TypeNode::Kind::Function:
            return -1; // function type has no size
    }
    return -1;
}

// Resolve a TypeNode for a sizeof/_Alignof operand that recorded raw type
// specifiers (`typeSpecs`). Builds a minimal TypeNode; returns null when the
// specifiers reference something we cannot size here (typedef-name, etc.).
static TypeNodePtr typeNodeFromSpecs(const DeclarationSpecifiers &specs) {
    if (specs.typeSpecifiers.empty()) return nullptr;
    const auto &ts = specs.typeSpecifiers.front();
    auto tn = std::make_shared<TypeNode>();
    if (ts.kind == DeclarationSpecifiers::TypeSpecifier::Kind::Simple) {
        tn->kind = TypeNode::Kind::Builtin;
        tn->simple = ts.simple;
        return tn;
    }
    if (ts.kind == DeclarationSpecifiers::TypeSpecifier::Kind::StructOrUnion && ts.su) {
        tn->kind = (ts.su->kind == StructOrUnionSpecifier::Kind::Struct)
                       ? TypeNode::Kind::Struct : TypeNode::Kind::Union;
        tn->su = ts.su;
        return tn;
    }
    if (ts.kind == DeclarationSpecifiers::TypeSpecifier::Kind::Enum) {
        tn->kind = TypeNode::Kind::Enum;
        return tn;
    }
    return nullptr; // typedef-name / atomic / other
}

// Apply an integer cast: convert `in` to the target arithmetic type described
// by `target`, performing truncation / sign behaviour where it matters for an
// ICE. Returns false if the cast target is not an integer/arithmetic type we
// can model (pointer/struct casts are not ICEs here).
static bool applyIntegerCast(const TypeNodePtr &target, ICEValue in, ICEValue &out) {
    if (!target) return false;
    // Strip qualifiers.
    TypeNodePtr t = target;
    while (t && t->kind == TypeNode::Kind::Qualified) t = t->pointee;
    if (!t) return false;
    if (t->kind == TypeNode::Kind::Enum) { out = {in.v, false}; return true; }
    if (t->kind != TypeNode::Kind::Builtin) return false; // pointer/struct cast: not an ICE
    if (t->simple.empty()) return false; // textual / typedef target

    bool hasUnsigned = false, hasLong = false, hasShort = false,
         hasChar = false, hasBool = false, hasFloat = false, hasDouble = false;
    for (auto s : t->simple) {
        if (s == STS::Unsigned) hasUnsigned = true;
        else if (s == STS::Long) hasLong = true;
        else if (s == STS::Short) hasShort = true;
        else if (s == STS::Char) hasChar = true;
        else if (s == STS::Bool) hasBool = true;
        else if (s == STS::Float) hasFloat = true;
        else if (s == STS::Double) hasDouble = true;
    }
    // Casts to floating types are not integer constant expressions.
    if (hasFloat || hasDouble) return false;

    if (hasBool) {
        // (_Bool) yields 0 or 1.
        out = { (in.v != 0) ? 1LL : 0LL, false };
        return true;
    }

    long long v = in.v;
    if (hasChar) {
        // Default char is signed in wvmcc unless `unsigned char`.
        if (hasUnsigned) v = (long long)(std::uint8_t)v;
        else v = (long long)(std::int8_t)v;
        out = { v, hasUnsigned };
        return true;
    }
    if (hasShort) {
        if (hasUnsigned) v = (long long)(std::uint16_t)v;
        else v = (long long)(std::int16_t)v;
        out = { v, hasUnsigned };
        return true;
    }
    if (hasLong) {
        // long / long long → 64-bit; just carry signedness.
        out = { v, hasUnsigned };
        return true;
    }
    // int (default): 32-bit.
    if (hasUnsigned) v = (long long)(std::uint32_t)v;
    else v = (long long)(std::int32_t)v;
    out = { v, hasUnsigned };
    return true;
}

// Canonical scalar-type identity, used both to size types and to match the
// controlling type of a `_Generic` selection against its associations. Spelling
// variants (`long unsigned` vs `unsigned long`, `signed` vs `int`) collapse to
// the same code; `long double` stays distinct from `double` for type-matching
// even though wvmcc lowers it to double.
enum class Scalar {
    Unknown, Void, Bool, Char, SChar, UChar, Short, UShort,
    Int, UInt, Long, ULong, LongLong, ULongLong, Float, Double, LongDouble
};

static Scalar canonScalar(const std::vector<STS> &simple) {
    if (simple.empty()) return Scalar::Int; // bare `int`
    bool uns = false, sig = false, ch = false, sh = false, vo = false,
         bo = false, fl = false, db = false;
    int lng = 0;
    for (auto s : simple) {
        switch (s) {
            case STS::Unsigned: uns = true; break;
            case STS::Signed:   sig = true; break;
            case STS::Char:     ch = true; break;
            case STS::Short:    sh = true; break;
            case STS::Long:     lng++; break;
            case STS::Void:     vo = true; break;
            case STS::Bool:     bo = true; break;
            case STS::Float:    fl = true; break;
            case STS::Double:   db = true; break;
            default: break;
        }
    }
    if (vo) return Scalar::Void;
    if (bo) return Scalar::Bool;
    if (db) return lng ? Scalar::LongDouble : Scalar::Double;
    if (fl) return Scalar::Float;
    if (ch) return uns ? Scalar::UChar : (sig ? Scalar::SChar : Scalar::Char);
    if (sh) return uns ? Scalar::UShort : Scalar::Short;
    if (lng >= 2) return uns ? Scalar::ULongLong : Scalar::LongLong;
    if (lng == 1) return uns ? Scalar::ULong : Scalar::Long;
    return uns ? Scalar::UInt : Scalar::Int;
}

// Canonical scalar identity of a TypeNode (Builtin/Enum), or Unknown when the
// type is not a self-contained scalar (pointer, struct, typedef-name, …).
static Scalar canonScalarOf(const TypeNodePtr &t) {
    if (!t) return Scalar::Unknown;
    TypeNodePtr c = t;
    while (c && c->kind == TypeNode::Kind::Qualified) c = c->pointee;
    if (!c) return Scalar::Unknown;
    if (c->kind == TypeNode::Kind::Enum) return Scalar::Int; // enum constants/type are int
    if (c->kind != TypeNode::Kind::Builtin) return Scalar::Unknown;
    if (c->simple.empty()) return Scalar::Unknown; // typedef-name / textual
    return canonScalar(c->simple);
}

static TypeNodePtr mkBuiltin(std::initializer_list<STS> specs) {
    auto tn = std::make_shared<TypeNode>();
    tn->kind = TypeNode::Kind::Builtin;
    tn->simple.assign(specs);
    return tn;
}

// Integer-promote a scalar type (6.3.1.1): types of lower rank than int become
// int. Used for sizeof/_Generic of promoted operands.
static TypeNodePtr promoteScalar(Scalar s) {
    switch (s) {
        case Scalar::Bool: case Scalar::Char: case Scalar::SChar:
        case Scalar::UChar: case Scalar::Short: case Scalar::UShort:
        case Scalar::Int:
            return mkBuiltin({STS::Int});
        case Scalar::UInt:      return mkBuiltin({STS::Unsigned});
        case Scalar::Long:      return mkBuiltin({STS::Long});
        case Scalar::ULong:     return mkBuiltin({STS::Unsigned, STS::Long});
        case Scalar::LongLong:  return mkBuiltin({STS::Long, STS::Long});
        case Scalar::ULongLong: return mkBuiltin({STS::Unsigned, STS::Long, STS::Long});
        case Scalar::Float:     return mkBuiltin({STS::Float});
        case Scalar::Double:    return mkBuiltin({STS::Double});
        case Scalar::LongDouble:return mkBuiltin({STS::Long, STS::Double});
        default:                return nullptr;
    }
}

static int scalarRank(Scalar s) {
    switch (s) {
        case Scalar::Int: case Scalar::UInt: return 1;
        case Scalar::Long: case Scalar::ULong: return 2;
        case Scalar::LongLong: case Scalar::ULongLong: return 3;
        case Scalar::Float: return 4;
        case Scalar::Double: return 5;
        case Scalar::LongDouble: return 6;
        default: return 0;
    }
}

// Infer the (self-contained) type of an expression used as the operand of
// `sizeof` or the controlling expression of `_Generic`. Returns null when the
// type needs information we do not have at parse time (an identifier's declared
// type, a member access, a typedef-name). The operand is never evaluated; only
// its type matters (6.5.3.4p2 / 6.5.1.1).
static TypeNodePtr inferExprType(const ExprPtr &e) {
    if (!e) return nullptr;
    switch (e->kind) {
        case Expr::Kind::Integer: {
            auto il = std::static_pointer_cast<IntegerLiteral>(e);
            // Reflect the literal's resolved rank (suffix or magnitude) so e.g.
            // `_Generic(0L, long: …)` selects the `long` association.
            std::vector<STS> specs;
            if (il->isUnsigned) specs.push_back(STS::Unsigned);
            if (il->isLongLong) { specs.push_back(STS::Long); specs.push_back(STS::Long); }
            else if (il->isLong) specs.push_back(STS::Long);
            if (!il->isUnsigned && specs.empty()) specs.push_back(STS::Int);
            auto tn = std::make_shared<TypeNode>();
            tn->kind = TypeNode::Kind::Builtin;
            tn->simple = std::move(specs);
            return tn;
        }
        case Expr::Kind::Char:
            return mkBuiltin({STS::Int}); // a character constant has type int (6.4.4.4p10)
        case Expr::Kind::Float: {
            auto fl = std::static_pointer_cast<FloatLiteral>(e);
            return fl->isFloat ? mkBuiltin({STS::Float}) : mkBuiltin({STS::Double});
        }
        case Expr::Kind::Cast: {
            auto ce = std::static_pointer_cast<CastExpr>(e);
            return ce->type;
        }
        case Expr::Kind::Sizeof:
        case Expr::Kind::AlignOf:
            return mkBuiltin({STS::Unsigned, STS::Long}); // size_t (LP64)
        case Expr::Kind::Unary: {
            auto ue = std::static_pointer_cast<UnaryExpr>(e);
            if (ue->op == "!") return mkBuiltin({STS::Int});
            if (ue->op == "-" || ue->op == "+" || ue->op == "~")
                return promoteScalar(canonScalarOf(inferExprType(ue->rhs)));
            if (ue->op == "&") {
                // The address-of operator yields a pointer regardless of the
                // operand's (possibly unknown) type. We only need "is a pointer"
                // for pointer subtraction below, so the pointee is left null.
                auto p = std::make_shared<TypeNode>();
                p->kind = TypeNode::Kind::Pointer;
                return p;
            }
            // `*deref` (and any other unary) needs the operand's real type, which
            // the standalone evaluator lacks — defer to the semantic resolver so
            // e.g. `sizeof *p` can be sized.
            if (g_typeResolver && *g_typeResolver) return (*g_typeResolver)(e);
            return nullptr;
        }
        case Expr::Kind::Ternary: {
            auto te = std::static_pointer_cast<TernaryExpr>(e);
            auto a = inferExprType(te->thenExpr);
            auto b = inferExprType(te->elseExpr);
            Scalar sa = canonScalarOf(a), sb = canonScalarOf(b);
            if (sa == Scalar::Unknown) return b;
            if (sb == Scalar::Unknown) return a;
            return scalarRank(sb) > scalarRank(sa) ? promoteScalar(sb) : promoteScalar(sa);
        }
        case Expr::Kind::Binary: {
            auto be = std::static_pointer_cast<BinaryExpr>(e);
            const std::string &op = be->op;
            // Relational / equality / logical operators yield int.
            if (op == "<" || op == ">" || op == "<=" || op == ">=" ||
                op == "==" || op == "!=" || op == "&&" || op == "||")
                return mkBuiltin({STS::Int});
            // Shift result has the (promoted) type of the left operand.
            if (op == "<<" || op == ">>")
                return promoteScalar(canonScalarOf(inferExprType(be->lhs)));
            // Pointer difference (pointer - pointer) has type ptrdiff_t, which
            // is `long` in the LP64 model. (pointer - integer stays a pointer,
            // which is not an integer type and so not an ICE operand here.)
            if (op == "-") {
                auto lt = inferExprType(be->lhs);
                auto rt = inferExprType(be->rhs);
                bool lp = lt && lt->kind == TypeNode::Kind::Pointer;
                bool rp = rt && rt->kind == TypeNode::Kind::Pointer;
                if (lp && rp) return mkBuiltin({STS::Long});
                if (lp || rp) return nullptr; // pointer ± integer: not an integer type
            }
            // Other arithmetic/bitwise: usual arithmetic conversions — the
            // higher-ranked promoted operand type wins.
            Scalar l = canonScalarOf(inferExprType(be->lhs));
            Scalar r = canonScalarOf(inferExprType(be->rhs));
            if (l == Scalar::Unknown || r == Scalar::Unknown) return nullptr;
            Scalar hi = scalarRank(r) > scalarRank(l) ? r : l;
            return promoteScalar(hi);
        }
        default:
            // #81: identifiers, member accesses, array subscripts, etc. carry no
            // type in the standalone evaluator. When the semantic pass has
            // installed a resolver, use it so `sizeof obj` / `sizeof a[0]` /
            // `sizeof s.m` can determine the operand's type.
            if (g_typeResolver && *g_typeResolver) return (*g_typeResolver)(e);
            return nullptr;
    }
}

// Fold an arithmetic constant expression to a double. Handles floating and
// integer constants and the arithmetic operators; returns nullopt for anything
// non-constant. This supports comparing floating constants in an ICE (see the
// relational branch of evalICE) — a wvmcc relaxation of the strict 6.6p6 rule
// (which only permits floating constants as the immediate operand of a cast),
// matching what the standard test suite expects.
static std::optional<double> foldDouble(const ExprPtr &e) {
    if (!e) return std::nullopt;
    switch (e->kind) {
        case Expr::Kind::Float:
            return std::static_pointer_cast<FloatLiteral>(e)->value;
        case Expr::Kind::Integer: {
            auto il = std::static_pointer_cast<IntegerLiteral>(e);
            return il->isUnsigned ? (double)(unsigned long long)il->value
                                  : (double)il->value;
        }
        case Expr::Kind::Char:
            return (double)std::static_pointer_cast<CharLiteral>(e)->value;
        case Expr::Kind::Unary: {
            auto ue = std::static_pointer_cast<UnaryExpr>(e);
            auto v = foldDouble(ue->rhs);
            if (!v) return std::nullopt;
            if (ue->op == "+") return *v;
            if (ue->op == "-") return -*v;
            return std::nullopt;
        }
        case Expr::Kind::Cast: {
            auto ce = std::static_pointer_cast<CastExpr>(e);
            Scalar s = canonScalarOf(ce->type);
            // Only a cast to a floating type stays in the floating domain; a
            // cast to an integer type is handled by the integer evaluator.
            if (s == Scalar::Float || s == Scalar::Double || s == Scalar::LongDouble)
                return foldDouble(ce->expr);
            return std::nullopt;
        }
        case Expr::Kind::Binary: {
            auto be = std::static_pointer_cast<BinaryExpr>(e);
            auto l = foldDouble(be->lhs);
            auto r = foldDouble(be->rhs);
            if (!l || !r) return std::nullopt;
            if (be->op == "+") return *l + *r;
            if (be->op == "-") return *l - *r;
            if (be->op == "*") return *l * *r;
            if (be->op == "/") { if (*r == 0.0) return std::nullopt; return *l / *r; }
            return std::nullopt;
        }
        case Expr::Kind::Ternary: {
            auto te = std::static_pointer_cast<TernaryExpr>(e);
            auto c = evalICE(te->cond);
            if (!c) return std::nullopt;
            return c->v ? foldDouble(te->thenExpr) : foldDouble(te->elseExpr);
        }
        default:
            return std::nullopt;
    }
}

// Core recursive evaluator returning the signed/unsigned-tagged value.
static std::optional<ICEValue> evalICE(const ExprPtr &e) {
    if (!e) return std::nullopt;
    switch (e->kind) {
        case Expr::Kind::Integer: {
            auto il = std::static_pointer_cast<IntegerLiteral>(e);
            return ICEValue{ il->value, il->isUnsigned };
        }
        case Expr::Kind::Char: {
            auto cl = std::static_pointer_cast<CharLiteral>(e);
            return ICEValue{ (long long)cl->value, false };
        }
        case Expr::Kind::Unary: {
            auto ue = std::static_pointer_cast<UnaryExpr>(e);
            if (!ue->rhs) return std::nullopt;
            if (ue->op == "++" || ue->op == "--") return std::nullopt;
            // address-of / deref / etc. are not integer constants
            auto v = evalICE(ue->rhs);
            if (!v.has_value()) return std::nullopt;
            if (ue->op == "-") return ICEValue{ -v->v, v->isUnsigned };
            if (ue->op == "+") return *v;
            if (ue->op == "~") return ICEValue{ ~v->v, v->isUnsigned };
            if (ue->op == "!") return ICEValue{ (long long)(!v->v), false };
            return std::nullopt;
        }
        case Expr::Kind::PostfixUnary:
            return std::nullopt;
        case Expr::Kind::Call:
            return std::nullopt;
        case Expr::Kind::Cast: {
            auto ce = std::static_pointer_cast<CastExpr>(e);
            ICEValue inner;
            // A floating constant is permitted in an ICE only as the operand of
            // a cast to an integer type (6.6p6). Evaluate it here rather than in
            // the general evaluator so floats cannot leak into bare arithmetic.
            if (ce->expr && ce->expr->kind == Expr::Kind::Float) {
                auto fl = std::static_pointer_cast<FloatLiteral>(ce->expr);
                // A cast to _Bool yields 0/1 by comparing the (un-truncated)
                // value to 0, so a nonzero fraction like 0.5 becomes 1. Any
                // other integer target truncates toward zero.
                if (canonScalarOf(ce->type) == Scalar::Bool)
                    return ICEValue{ fl->value != 0.0 ? 1LL : 0LL, false };
                inner = ICEValue{ (long long)fl->value, false };
            } else {
                auto iv = evalICE(ce->expr);
                if (!iv.has_value()) return std::nullopt;
                inner = *iv;
            }
            ICEValue out;
            if (!applyIntegerCast(ce->type, inner, out)) return std::nullopt;
            return out;
        }
        case Expr::Kind::Sizeof: {
            auto so = std::static_pointer_cast<SizeofExpr>(e);
            TypeNodePtr opType;
            if (so->typeSpecs.has_value()) {
                opType = typeNodeFromSpecs(*so->typeSpecs);
            } else if (so->type.has_value()) {
                opType = *so->type;
            } else if (so->expr) {
                // sizeof of an expression (6.5.3.4p2): the operand is not
                // evaluated; only its type determines the size.
                if (so->expr->kind == Expr::Kind::String) {
                    // String literal has type char[len+1]; value is the decoded
                    // content (escapes resolved), so size is value.size()+1.
                    auto sl = std::static_pointer_cast<StringLiteral>(so->expr);
                    return ICEValue{ (long long)sl->value.size() + 1, true };
                }
                opType = inferExprType(so->expr);
            } else {
                return std::nullopt;
            }
            if (!opType) return std::nullopt;
            long long s = typeNodeSize(opType, /*wantAlign=*/false);
            if (s < 0) return std::nullopt; // incomplete / VLA / unresolvable
            // size_t is unsigned.
            return ICEValue{ s, true };
        }
        case Expr::Kind::AlignOf: {
            auto ao = std::static_pointer_cast<AlignOfExpr>(e);
            TypeNodePtr opType;
            if (ao->typeSpecs.has_value()) {
                opType = typeNodeFromSpecs(*ao->typeSpecs);
            } else {
                opType = ao->type;
            }
            if (!opType) return std::nullopt;
            long long a = typeNodeSize(opType, /*wantAlign=*/true);
            if (a < 0) return std::nullopt;
            return ICEValue{ a, true };
        }
        case Expr::Kind::Binary: {
            auto be = std::static_pointer_cast<BinaryExpr>(e);
            if (!be->lhs || !be->rhs) return std::nullopt;
            if (be->op == ",") return std::nullopt;
            static const char *assignOps[] = {"=","*=","/=","%=","+=","-=","<<=",">>=","&=","^=","|="};
            for (auto aop : assignOps) if (be->op == aop) return std::nullopt;

            // Short-circuit operators (6.6p3 / footnote): a subexpression that is
            // not evaluated need not be a constant expression, so the right
            // operand of `||`/`&&` is only required to be an ICE when the left
            // operand does not already decide the result. This makes e.g.
            // `2 || 1/0` a valid ICE with value 1 (the `1/0` is never evaluated).
            if (be->op == "||" || be->op == "&&") {
                auto L = evalICE(be->lhs);
                if (!L.has_value()) return std::nullopt;
                const bool lTrue = (L->v != 0);
                if (be->op == "||" && lTrue)  return ICEValue{ 1, false };
                if (be->op == "&&" && !lTrue) return ICEValue{ 0, false };
                auto R = evalICE(be->rhs);
                if (!R.has_value()) return std::nullopt;
                return ICEValue{ (long long)(R->v != 0), false };
            }

            // Relational / equality on floating constants: when the operands are
            // not integer-evaluable, fall back to a double fold so e.g.
            // `FLT_EPSILON > 0` and `1.0 + DBL_EPSILON > 1.0` are constant. The
            // result of a comparison is always int, so it is a valid ICE value.
            {
                static const char *relOps[] = {"<",">","<=",">=","==","!="};
                bool isRel = false;
                for (auto o : relOps) if (be->op == o) { isRel = true; break; }
                if (isRel && !(evalICE(be->lhs) && evalICE(be->rhs))) {
                    auto lf = foldDouble(be->lhs);
                    auto rf = foldDouble(be->rhs);
                    if (!lf || !rf) return std::nullopt;
                    double a = *lf, b = *rf;
                    bool res;
                    if (be->op == "<") res = a < b;
                    else if (be->op == ">") res = a > b;
                    else if (be->op == "<=") res = a <= b;
                    else if (be->op == ">=") res = a >= b;
                    else if (be->op == "==") res = a == b;
                    else res = a != b;
                    return ICEValue{ (long long)res, false };
                }
            }

            auto L = evalICE(be->lhs);
            auto R = evalICE(be->rhs);
            if (!L.has_value() || !R.has_value()) return std::nullopt;
            const long long l = L->v;
            const long long r = R->v;
            // Result of the usual arithmetic conversions: unsigned if either
            // operand is unsigned (both are 64-bit here in the LP64 model).
            const bool resU = L->isUnsigned || R->isUnsigned;

            if (be->op == "+") return ICEValue{ l + r, resU };
            if (be->op == "-") return ICEValue{ l - r, resU };
            if (be->op == "*") return ICEValue{ l * r, resU };
            if (be->op == "/") {
                if (r == 0) return std::nullopt;
                if (resU) return ICEValue{ (long long)((unsigned long long)l / (unsigned long long)r), true };
                return ICEValue{ l / r, false };
            }
            if (be->op == "%") {
                if (r == 0) return std::nullopt;
                if (resU) return ICEValue{ (long long)((unsigned long long)l % (unsigned long long)r), true };
                return ICEValue{ l % r, false };
            }
            if (be->op == "<<") {
                // Result has the type of the (promoted) left operand.
                return ICEValue{ (long long)((unsigned long long)l << (unsigned long long)r), L->isUnsigned };
            }
            if (be->op == ">>") {
                if (L->isUnsigned) return ICEValue{ (long long)((unsigned long long)l >> (unsigned long long)r), true };
                return ICEValue{ l >> r, false };
            }
            if (be->op == "&") return ICEValue{ l & r, resU };
            if (be->op == "|") return ICEValue{ l | r, resU };
            if (be->op == "^") return ICEValue{ l ^ r, resU };
            // Relational / equality: comparisons yield int (signed).
            if (be->op == "<")
                return ICEValue{ (long long)(resU ? (unsigned long long)l < (unsigned long long)r : l < r), false };
            if (be->op == ">")
                return ICEValue{ (long long)(resU ? (unsigned long long)l > (unsigned long long)r : l > r), false };
            if (be->op == "<=")
                return ICEValue{ (long long)(resU ? (unsigned long long)l <= (unsigned long long)r : l <= r), false };
            if (be->op == ">=")
                return ICEValue{ (long long)(resU ? (unsigned long long)l >= (unsigned long long)r : l >= r), false };
            if (be->op == "==") return ICEValue{ (long long)(l == r), false };
            if (be->op == "!=") return ICEValue{ (long long)(l != r), false };
            // `&&` / `||` handled above with short-circuit semantics.
            return std::nullopt;
        }
        case Expr::Kind::Ternary: {
            auto te = std::static_pointer_cast<TernaryExpr>(e);
            if (!te->cond) return std::nullopt;
            auto c = evalICE(te->cond);
            if (!c.has_value()) return std::nullopt;
            return c->v ? evalICE(te->thenExpr) : evalICE(te->elseExpr);
        }
        case Expr::Kind::GenericSelection: {
            // _Generic (6.5.1.1): select the association whose type is
            // compatible with the controlling expression's type (which is not
            // evaluated), then evaluate the selected expression as the ICE.
            auto ge = std::static_pointer_cast<GenericSelectionExpr>(e);
            Scalar ctrl = canonScalarOf(inferExprType(ge->controlling));
            if (ctrl == Scalar::Unknown) return std::nullopt;
            const GenericAssociation *chosen = nullptr;
            const GenericAssociation *deflt = nullptr;
            for (const auto &a : ge->assocs) {
                if (a.isDefault) { deflt = &a; continue; }
                if (canonScalarOf(a.type) == ctrl) { chosen = &a; break; }
            }
            if (!chosen) chosen = deflt;
            if (!chosen) return std::nullopt;
            return evalICE(chosen->expr);
        }
        case Expr::Kind::Index: {
            // Subscripting a string literal with a constant index, e.g.
            // `"&="[0]`, is an integer constant expression yielding the indexed
            // character (the byte at value.size() is the terminating '\0').
            auto ix = std::static_pointer_cast<IndexExpr>(e);
            ExprPtr strE, idxE;
            if (ix->base && ix->base->kind == Expr::Kind::String) { strE = ix->base; idxE = ix->index; }
            else if (ix->index && ix->index->kind == Expr::Kind::String) { strE = ix->index; idxE = ix->base; }
            else return std::nullopt;
            auto k = evalICE(idxE);
            if (!k.has_value()) return std::nullopt;
            auto sl = std::static_pointer_cast<StringLiteral>(strE);
            long long i = k->v;
            if (i < 0 || (size_t)i > sl->value.size()) return std::nullopt; // out of range
            // The element type is `char` (signed by default in wvmcc).
            char c = (i == (long long)sl->value.size()) ? '\0' : sl->value[(size_t)i];
            return ICEValue{ (long long)(std::int8_t)c, false };
        }
        default:
            return std::nullopt;
    }
}

} // namespace

std::optional<long long> ConstExprEvaluator::evalIntegerConstantExpr(const ExprPtr &e) {
    auto r = evalICE(e);
    if (!r.has_value()) return std::nullopt;
    return r->v;
}

ConstExprEvaluator::ResolverScope::ResolverScope(TypeResolver r) {
    static TypeResolver storage;
    storage = std::move(r);
    g_typeResolver = &storage;
}
ConstExprEvaluator::ResolverScope::~ResolverScope() {
    g_typeResolver = nullptr;
}

std::optional<long long> ConstExprEvaluator::structMemberOffset(
    const TypeNodePtr &structType, const std::string &member) {
    if (!structType) return std::nullopt;
    auto su = structType->su;
    if (!su || !su->hasBody) return std::nullopt;
    const bool isUnion = (su->kind == StructOrUnionSpecifier::Kind::Union);
    // Name of a (possibly pointer/array-adorned) declarator.
    auto declName = [](DeclaratorPtr d) -> std::string {
        for (; d; d = (d->inner.has_value() ? *d->inner : nullptr))
            if (!d->id.name.empty()) return d->id.name;
        return "";
    };
    long long offset = 0;
    for (const auto &m : su->members) {
        // Bit-fields are not modelled — bail (the offset would be unreliable).
        for (const auto &sd : m.declarators)
            if (sd.bitfieldWidth.has_value()) return std::nullopt;
        // Member base type from the first type specifier (mirrors suByteSize).
        TypeNodePtr baseType;
        for (const auto &ts : m.specifiers.typeSpecifiers) {
            if (ts.kind == DeclarationSpecifiers::TypeSpecifier::Kind::Simple) {
                baseType = std::make_shared<TypeNode>();
                baseType->kind = TypeNode::Kind::Builtin; baseType->simple = ts.simple;
            } else if (ts.kind == DeclarationSpecifiers::TypeSpecifier::Kind::StructOrUnion && ts.su) {
                baseType = std::make_shared<TypeNode>();
                baseType->kind = (ts.su->kind == StructOrUnionSpecifier::Kind::Struct)
                                     ? TypeNode::Kind::Struct : TypeNode::Kind::Union;
                baseType->su = ts.su;
            } else if (ts.kind == DeclarationSpecifiers::TypeSpecifier::Kind::Enum) {
                baseType = std::make_shared<TypeNode>();
                baseType->kind = TypeNode::Kind::Enum;
            } else {
                return std::nullopt; // typedef-name / atomic: not self-computable
            }
            break;
        }
        if (!baseType) return std::nullopt;
        auto wrapped = [&](const StructDeclarator &sd) -> TypeNodePtr {
            TypeNodePtr ty = baseType;
            for (DeclaratorPtr d = sd.declarator; d;
                 d = (d->inner.has_value() ? *d->inner : nullptr)) {
                if (d->kind == Declarator::Kind::Pointer) {
                    auto p = std::make_shared<TypeNode>();
                    p->kind = TypeNode::Kind::Pointer; p->pointee = ty; ty = p;
                } else if (d->kind == Declarator::Kind::Array) {
                    auto a = std::make_shared<TypeNode>();
                    a->kind = TypeNode::Kind::Array; a->element = ty;
                    if (d->array.size.has_value()) a->sizeExpr = d->array.size;
                    else return nullptr; // flexible array member
                    ty = a;
                } else if (d->kind == Declarator::Kind::Function) {
                    return nullptr;
                }
            }
            return ty;
        };
        if (m.declarators.empty()) {
            // Anonymous member: layout-relevant but unnamed; advance the offset.
            long long sz = typeNodeSize(baseType, false);
            long long al = typeNodeSize(baseType, true);
            if (sz < 0 || al < 0) return std::nullopt;
            if (!isUnion) {
                if (al > 0 && offset % al != 0) offset += al - offset % al;
                offset += sz;
            }
            continue;
        }
        for (const auto &sd : m.declarators) {
            auto ty = wrapped(sd);
            if (!ty) return std::nullopt;
            long long sz = typeNodeSize(ty, false);
            long long al = typeNodeSize(ty, true);
            if (sz < 0 || al < 0) return std::nullopt;
            long long memOff;
            if (isUnion) {
                memOff = 0;
            } else {
                if (al > 0 && offset % al != 0) offset += al - offset % al;
                memOff = offset;
            }
            if (declName(sd.declarator) == member) return memOff;
            if (!isUnion) offset = memOff + sz;
        }
    }
    return std::nullopt; // member not found
}

bool ConstExprEvaluator::dependsOnUnresolvedSizeof(const ExprPtr &e) {
    if (!e) return false;
    using K = Expr::Kind;
    switch (e->kind) {
        case K::Sizeof: {
            auto so = std::static_pointer_cast<SizeofExpr>(e);
            // A type-name operand (`sizeof(int)`) resolves without symbols; an
            // expression operand other than a string literal needs one.
            if (so->typeSpecs.has_value()) return false;
            if (so->expr && so->expr->kind != K::String) return true;
            return false;
        }
        case K::AlignOf: {
            auto ao = std::static_pointer_cast<AlignOfExpr>(e);
            // _Alignof always takes a type-name in C, so it resolves without
            // symbols; nothing to defer.
            (void)ao;
            return false;
        }
        case K::Unary:
            return dependsOnUnresolvedSizeof(std::static_pointer_cast<UnaryExpr>(e)->rhs);
        case K::Cast:
            return dependsOnUnresolvedSizeof(std::static_pointer_cast<CastExpr>(e)->expr);
        case K::Binary: {
            auto be = std::static_pointer_cast<BinaryExpr>(e);
            return dependsOnUnresolvedSizeof(be->lhs) || dependsOnUnresolvedSizeof(be->rhs);
        }
        case K::Ternary: {
            auto te = std::static_pointer_cast<TernaryExpr>(e);
            return dependsOnUnresolvedSizeof(te->cond)
                || dependsOnUnresolvedSizeof(te->thenExpr)
                || dependsOnUnresolvedSizeof(te->elseExpr);
        }
        case K::GenericSelection:
            // A `_Generic` selection picks an association by the controlling
            // expression's *type* (6.5.1.1), which for anything but a literal or
            // cast needs the symbol table the parser-time evaluator lacks. Defer
            // to the semantic pass (which installs a type resolver). Only reached
            // when parse-time evaluation already failed, so this never hides a
            // genuinely non-constant assertion — semantic re-checks and errors.
            return true;
        default:
            return false;
    }
}

} // namespace wvmcc::parser
