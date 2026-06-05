// #80: a preprocessor or parser error must force a non-zero exit status.
//
// Before this fix, pp/parser diagnostics were printed to stderr but did not
// affect the exit code — a TU that failed only at preprocessing (e.g. a live
// `#error`) or parsing could still exit 0, which would let the standard test
// suite's `compile-fail` rows (M2-19 / #76) wrongly pass.
//
// Exit-code contract: 0 = success, 1 = compilation failure, 2 = CLI-arg error.
// Warnings alone must NOT force a non-zero exit.
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

static const std::vector<std::string> candidates = {
    "./wvmcc", "../wvmcc", "./tests/wvmcc", "../tests/wvmcc",
    "../../wvmcc", "../../../wvmcc"
};

static std::string locate_wvmcc() {
    for (const auto& c : candidates) {
        if (std::FILE* f = std::fopen(c.c_str(), "r")) {
            std::fclose(f);
            return c;
        }
    }
    return {};
}

static int run(const std::string& cmd) {
    int rc = std::system(cmd.c_str());
    return (rc == -1) ? 127 : (rc >> 8);
}

static void write_file(const std::string& path, const std::string& contents) {
    std::ofstream ofs(path);
    ofs << contents;
}

int main() {
    std::string exe = locate_wvmcc();
    if (exe.empty()) {
        std::cerr << "cannot locate wvmcc\n";
        return 4;
    }

    const std::string out = "diag_exit_out.wasm";
    int failures = 0;

    auto expect = [&](const std::string& label, const std::string& cmd,
                      bool wantNonZero, int wantExact = -1) {
        int rc = run(cmd + " 2>/dev/null >/dev/null");
        bool ok = (wantExact >= 0) ? (rc == wantExact)
                                   : (wantNonZero ? rc != 0 : rc == 0);
        if (!ok) {
            std::cerr << "FAIL: " << label << " (got exit=" << rc << ")\n";
            ++failures;
        }
    };

    // 1. A live `#error` must fail the compile (was the core bug: exited 0).
    write_file("diag_pperr.c", "#error boom\nint main(void){return 0;}\n");
    expect("#error -> non-zero",
           exe + " -ffreestanding diag_pperr.c -o " + out, true);

    // 2. Same in preprocessor-only (-E) mode.
    expect("#error -E -> non-zero",
           exe + " -E diag_pperr.c", true);

    // 3. A parse/syntax error must fail the compile. `return <expr>;` in a
    //    void function is a parser-emitted error (Parser.cpp).
    write_file("diag_parseerr.c",
               "void f(void){ return 1; }\nint main(void){return 0;}\n");
    expect("parse error -> non-zero",
           exe + " -ffreestanding diag_parseerr.c -o " + out, true);

    // 4. A warning alone must NOT force a non-zero exit.
    write_file("diag_warn.c", "#warning heads up\nint main(void){return 0;}\n");
    expect("warning-only -> zero",
           exe + " -ffreestanding diag_warn.c -o " + out, false);

    // 5. A well-formed program still exits 0 (compile and -E).
    write_file("diag_ok.c", "int main(void){return 0;}\n");
    expect("well-formed -> zero",
           exe + " -ffreestanding diag_ok.c -o " + out, false);
    expect("well-formed -E -> zero",
           exe + " -E diag_ok.c", false);

    // 6. A CLI-argument error stays exit 2 (distinct from compile failure).
    expect("no input -> exit 2",
           exe + " -ffreestanding", false, /*wantExact=*/2);

    for (const char* f : {"diag_pperr.c", "diag_parseerr.c", "diag_warn.c",
                          "diag_ok.c"}) {
        std::remove(f);
    }
    std::remove(out.c_str());

    if (failures == 0) {
        std::cout << "all diagnostic-exit-status tests passed\n";
    }
    return failures == 0 ? 0 : 1;
}
