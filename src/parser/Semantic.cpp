#include "Semantic.hpp"

#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <string>
#include "ConstExprEval.hpp"

using wvmcc::Diagnostic;


namespace wvmcc::parser {
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
// Create a compact signature string for a declaration's type/specifiers/declarator
static std::string signatureForDeclaration(const DeclarationPtr &d) {
    if (!d) return std::string();
    std::string s;
    // specifiers
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
    return s;
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
        auto it = declaredSignatures.find(name);
        if (it != declaredSignatures.end() && it->second != sig && curDiagnostics) {
            Diagnostic diag;
            diag.severity = Diagnostic::Severity::Error;
            diag.message = "incompatible declaration for '" + name + "'";
            diag.span = f->declarator->span;
            curDiagnostics->push_back(std::move(diag));
        } else {
            declaredSignatures[name] = sig;
        }
        if (!f->specifiers.hasStorage(wvmcc::parser::StorageClass::Static)) recordDef(name, f->declarator->span);
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
        auto it = declaredSignatures.find(name);
        if (it != declaredSignatures.end() && it->second != sig && curDiagnostics) {
            Diagnostic diag;
            diag.severity = Diagnostic::Severity::Error;
            diag.message = "incompatible declaration for '" + name + "'";
            diag.span = d->declarator->span;
            curDiagnostics->push_back(std::move(diag));
        } else {
            declaredSignatures[name] = sig;
        }
    }
    
    if (d->declarator) {
        std::string rname = declaratorName(d->declarator);
        if (!rname.empty()) {
            bool isDef = false;
            if (d->specifiers.hasStorage(wvmcc::parser::StorageClass::Extern) && d->initializer.has_value()) isDef = true;
            if (!d->specifiers.hasStorage(wvmcc::parser::StorageClass::Extern)) isDef = true;
            if (isDef && !d->specifiers.hasStorage(wvmcc::parser::StorageClass::Static)) {
                recordDef(rname, d->declarator->span);
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
            Diagnostic diag;
            diag.severity = Diagnostic::Severity::Warning;
            diag.message = "identifier '" + name + "' used but no external definition in this translation unit";
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

void Semantic::checkExternal(const ExternalDeclPtr &e, std::vector<wvmcc::Diagnostic> &diagnostics) {
    if (!e) return;
    if (std::holds_alternative<FunctionDefPtr>(e->decl)) {
        checkFunction(std::get<FunctionDefPtr>(e->decl), diagnostics);
    } else if (std::holds_alternative<DeclarationPtr>(e->decl)) {
        checkDeclaration(std::get<DeclarationPtr>(e->decl), diagnostics);
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
    if (d->declarator->id.name.empty()) {
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
