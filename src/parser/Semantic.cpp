#include "Semantic.hpp"

#include <iostream>

using wvmcc::Diagnostic;


namespace wvmcc::parser {

void Semantic::run(const TranslationUnitPtr &tu, std::vector<wvmcc::Diagnostic> &diagnostics) {
    if (!tu) return;
    for (auto &ext : tu->externals) {
        checkExternal(ext, diagnostics);
    }
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
    if (f->declarator->id.name.empty()) {
        Diagnostic diag;
        diag.severity = Diagnostic::Severity::Error;
        diag.message = "function with empty name";
        diag.span = f->declarator->span;
        diagnostics.push_back(std::move(diag));
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
}

} // namespace wvmcc::parser
