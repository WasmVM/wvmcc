// Unit tests for the _Pragma operator (ISO C17 6.10.9) — #108 — and the
// silently-accepted `#pragma STDC FP_CONTRACT` (6.10.6p2 / 6.5p8) — #113.
#include "pp/Preprocessor.hpp"
#include <fstream>
#include <string>
#include <iostream>
#include <vector>

using namespace wvmcc;

namespace {

struct PPResult {
    std::vector<PPToken> tokens;
    std::vector<Diagnostic> diags;
};

// Write `src` to a temp file, drain the preprocessor, and return tokens+diags.
int runPP(const std::string& name, const std::string& src, PPResult& out) {
    {
        std::ofstream ofs(name);
        ofs << src;
    }
    Preprocessor pp;
    if (!pp.open(name)) {
        std::remove(name.c_str());
        std::cerr << name << ": failed to open input\n";
        return 1;
    }
    while (auto t = pp.next()) out.tokens.push_back(*t);
    out.diags = pp.getDiagnostics();
    std::remove(name.c_str());
    return 0;
}

bool hasDiag(const PPResult& r, Diagnostic::Severity sev, const std::string& needle) {
    for (const auto& d : r.diags) {
        if (d.severity == sev && d.message.find(needle) != std::string::npos) return true;
    }
    return false;
}

int countSignificant(const PPResult& r) {
    int n = 0;
    for (const auto& t : r.tokens) {
        if (t.kind != PPTokenKind::Whitespace && t.kind != PPTokenKind::Newline) n++;
    }
    return n;
}

} // namespace

// _Pragma("...") with an unknown pragma behaves exactly like the equivalent
// #pragma directive: warn-and-ignore, and the operator produces no tokens.
static int test_pragma_operator_unknown_warns() {
    PPResult r;
    if (runPP("temp_pragma_op1.c", "_Pragma(\"frobnicate all\")\nint x;\n", r)) return 1;
    if (!hasDiag(r, Diagnostic::Severity::Warning, "#pragma: frobnicate all")) {
        std::cerr << "expected the unknown-pragma warning\n"; return 2;
    }
    if (countSignificant(r) != 3) {  // int x ;
        std::cerr << "expected _Pragma to emit no tokens, got " << countSignificant(r) << "\n"; return 3;
    }
    return 0;
}

// 6.10.9p2 destringize: \" -> " and \\ -> \ (other escapes untouched).
static int test_pragma_operator_destringize() {
    PPResult r;
    if (runPP("temp_pragma_op2.c",
              "_Pragma(\"msg \\\"quoted\\\" back\\\\slash\")\n", r)) return 1;
    if (!hasDiag(r, Diagnostic::Severity::Warning, "#pragma: msg \"quoted\" back\\slash")) {
        std::cerr << "destringize mismatch; got:\n";
        for (const auto& d : r.diags) std::cerr << "  " << d.message << "\n";
        return 2;
    }
    return 0;
}

// _Pragma("once") shares #pragma once's include-guard semantics.
static int test_pragma_operator_once() {
    {
        std::ofstream ofs("temp_pragma_op_hdr.h");
        ofs << "_Pragma(\"once\")\nint counter;\n";
    }
    PPResult r;
    int rc = runPP("temp_pragma_op3.c",
                   "#include \"temp_pragma_op_hdr.h\"\n#include \"temp_pragma_op_hdr.h\"\n", r);
    std::remove("temp_pragma_op_hdr.h");
    if (rc) return 1;
    int counters = 0;
    for (const auto& t : r.tokens) {
        if (t.kind == PPTokenKind::Identifier && t.lexeme == "counter") counters++;
    }
    if (counters != 1) {
        std::cerr << "expected the header body once, saw 'counter' x" << counters << "\n"; return 2;
    }
    return 0;
}

// The common idiom: _Pragma produced by macro expansion, with a stringified arg.
static int test_pragma_operator_from_macro() {
    PPResult r;
    if (runPP("temp_pragma_op4.c",
              "#define DO_PRAGMA(x) _Pragma(#x)\nDO_PRAGMA(weird thing)\n", r)) return 1;
    if (!hasDiag(r, Diagnostic::Severity::Warning, "#pragma: weird thing")) {
        std::cerr << "expected the macro-produced pragma to be executed\n"; return 2;
    }
    return 0;
}

// A non-string operand is a constraint violation.
static int test_pragma_operator_bad_operand() {
    PPResult r;
    if (runPP("temp_pragma_op5.c", "_Pragma(42)\n", r)) return 1;
    if (!hasDiag(r, Diagnostic::Severity::Error, "string literal")) {
        std::cerr << "expected an error for a non-string operand\n"; return 2;
    }
    return 0;
}

// #113: all three FP_CONTRACT states are accepted silently, via both the
// directive and the operator; anything malformed still warns.
static int test_stdc_fp_contract_accepted() {
    PPResult r;
    if (runPP("temp_pragma_op6.c",
              "#pragma STDC FP_CONTRACT ON\n"
              "#pragma STDC FP_CONTRACT OFF\n"
              "_Pragma(\"STDC FP_CONTRACT DEFAULT\")\n", r)) return 1;
    if (!r.diags.empty()) {
        std::cerr << "expected FP_CONTRACT to be accepted silently; got: "
                  << r.diags[0].message << "\n";
        return 2;
    }
    PPResult r2;
    if (runPP("temp_pragma_op7.c", "#pragma STDC FP_CONTRACT SIDEWAYS\n", r2)) return 3;
    if (!hasDiag(r2, Diagnostic::Severity::Warning, "#pragma: STDC FP_CONTRACT SIDEWAYS")) {
        std::cerr << "expected a warning for a malformed FP_CONTRACT state\n"; return 4;
    }
    return 0;
}

#define RUN(fn) \
    do { \
        int r = fn(); \
        if (r != 0) { std::cerr << #fn " FAILED (code " << r << ")\n"; return r; } \
        std::cout << #fn " passed\n"; \
    } while (0)

int main() {
    RUN(test_pragma_operator_unknown_warns);
    RUN(test_pragma_operator_destringize);
    RUN(test_pragma_operator_once);
    RUN(test_pragma_operator_from_macro);
    RUN(test_pragma_operator_bad_operand);
    RUN(test_stdc_fp_contract_accepted);
    std::cout << "All _Pragma operator tests passed!\n";
    return 0;
}
