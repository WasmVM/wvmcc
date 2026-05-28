#pragma once

#include "Linker.hpp"
#include "../common.hpp"
#include <WasmVM.hpp>
#include <string>
#include <vector>

namespace wvmcc::link {

// Per-link mutable state. Each phase reads/mutates this and the output
// module. Subsequent issues (M2-L2..L10) add fields here as needed.
struct LinkContext {
    LinkOptions opts;

    // Inputs in command-line order, already canonicalized: each archive has
    // been resolved on disk; in-memory modules carry their origin string.
    std::vector<LinkInput> inputs;

    // The module under construction. Phases append to it and rewrite it
    // in place.
    WasmVM::WasmModule output;

    // Diagnostics collected across phases. The driver flushes them.
    std::vector<wvmcc::Diagnostic> diagnostics;

    // Verbose log lines (printed if opts.verbose).
    std::vector<std::string> log;

    void note(std::string msg) { log.push_back(std::move(msg)); }

    void error(std::string msg) {
        wvmcc::Diagnostic d;
        d.severity = wvmcc::Diagnostic::Severity::Error;
        d.message = std::move(msg);
        diagnostics.push_back(std::move(d));
    }

    bool hasErrors() const {
        for (const auto& d : diagnostics) {
            if (d.severity == wvmcc::Diagnostic::Severity::Error) return true;
        }
        return false;
    }
};

} // namespace wvmcc::link
