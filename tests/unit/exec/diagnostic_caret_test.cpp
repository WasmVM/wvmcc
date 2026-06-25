// #28: enhanced diagnostics — file:line:col header, a source-context snippet
// with a `^` caret, and an actionable `note:` hint. Also re-checks the #80
// contract that a warning alone does not force a non-zero exit.
//
// The diagnostic format is intentionally not asserted by the `standard`
// compile-fail suite (message text has no stable IDs), so this test guards the
// human-facing shape directly by scanning the compiler's stderr.
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
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

static void write_file(const std::string& path, const std::string& contents) {
    std::ofstream ofs(path);
    ofs << contents;
}

// Run `cmd`, capture stderr (redirected to a temp file), return exit code and
// fill `err` with the captured stderr text.
static int run_capture(const std::string& cmd, std::string& err) {
    const std::string errFile = "diag_caret_err.txt";
    int rc = std::system((cmd + " 2>" + errFile + " >/dev/null").c_str());
    std::ifstream ifs(errFile);
    std::stringstream ss; ss << ifs.rdbuf();
    err = ss.str();
    std::remove(errFile.c_str());
    return (rc == -1) ? 127 : (rc >> 8);
}

int main() {
    std::string exe = locate_wvmcc();
    if (exe.empty()) {
        std::cerr << "cannot locate wvmcc\n";
        return 4;
    }

    const std::string out = "diag_caret_out.wasm";
    int failures = 0;

    auto check = [&](const std::string& label, bool cond) {
        if (!cond) { std::cerr << "FAIL: " << label << "\n"; ++failures; }
    };

    // 1. A semantic type error (too many call arguments) must print a
    //    file:line:col header, the offending source line + caret, and the hint.
    write_file("diag_caret_argerr.c",
               "int add(int a, int b);\n"
               "int main(void){ return add(1, 2, 3); }\n");
    {
        std::string err;
        int rc = run_capture(exe + " -ffreestanding diag_caret_argerr.c -o " + out, err);
        check("arg-count -> non-zero exit", rc != 0);
        // header: "<file>:2:<col>: error: ..."
        check("arg-count -> file:line:col header",
              err.find("diag_caret_argerr.c:2:") != std::string::npos &&
              err.find(": error: too many arguments") != std::string::npos);
        // source-context snippet is reproduced
        check("arg-count -> snippet line",
              err.find("return add(1, 2, 3)") != std::string::npos);
        // a caret line exists
        check("arg-count -> caret", err.find('^') != std::string::npos);
        // the actionable hint
        check("arg-count -> hint note",
              err.find("note: function expects 2 arguments") != std::string::npos);
    }

    // 2. A parser/syntax error also carries a span -> header + caret.
    write_file("diag_caret_syn.c", "void f(void){ return 1; }\n");
    {
        std::string err;
        int rc = run_capture(exe + " -ffreestanding diag_caret_syn.c -o " + out, err);
        check("syntax -> non-zero exit", rc != 0);
        check("syntax -> file:line:col header",
              err.find("diag_caret_syn.c:1:") != std::string::npos &&
              err.find(": error: ") != std::string::npos);
        check("syntax -> caret", err.find('^') != std::string::npos);
    }

    // 3. Approach B: a diagnostic from inside an #included header must name the
    //    HEADER (not the TU) and render a caret from the header's own text.
    write_file("diag_caret_inc.h",
               "int two(int a, int b);\n"
               "static inline int boom(void){ return two(1, 2, 3); }\n");
    write_file("diag_caret_inc_main.c",
               "#include \"diag_caret_inc.h\"\n"
               "int main(void){ return boom(); }\n");
    {
        std::string err;
        int rc = run_capture(
            exe + " -ffreestanding diag_caret_inc_main.c -o " + out, err);
        check("include -> non-zero exit", rc != 0);
        // header is named with its line (2), not the .c file
        check("include -> header file:line header",
              err.find("diag_caret_inc.h:2:") != std::string::npos &&
              err.find(": error: too many arguments") != std::string::npos);
        // the caret snippet comes from the header's source text
        check("include -> header snippet",
              err.find("return two(1, 2, 3)") != std::string::npos &&
              err.find('^') != std::string::npos);
    }

    // 4. A warning alone must not force a non-zero exit (#80 contract).
    write_file("diag_caret_warn.c",
               "#warning heads up\nint main(void){return 0;}\n");
    {
        std::string err;
        int rc = run_capture(exe + " -ffreestanding diag_caret_warn.c -o " + out, err);
        check("warning-only -> zero exit", rc == 0);
    }

    for (const char* f : {"diag_caret_argerr.c", "diag_caret_syn.c",
                          "diag_caret_warn.c", "diag_caret_inc.h",
                          "diag_caret_inc_main.c"}) {
        std::remove(f);
    }
    std::remove(out.c_str());

    if (failures == 0) std::cout << "all diagnostic-caret tests passed\n";
    return failures == 0 ? 0 : 1;
}
