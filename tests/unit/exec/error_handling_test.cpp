// #27: comprehensive error handling.
//
// Contract under test:
//  - a call-site argument type mismatch is a named, located error (not a
//    silent mis-compile);
//  - one root cause produces one error — codegen does not stack cascading
//    secondary diagnostics on a construct sema/codegen already diagnosed;
//  - the compiler recovers per function: an error in one function does not
//    stop diagnosis of the rest of the TU;
//  - codegen diagnostics carry a file:line:col location;
//  - valid implicit conversions (null-constant->pointer, array decay,
//    pointer->_Bool, arithmetic promotions) are never rejected.
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

static int run(const std::string& cmd) {
    int rc = std::system(cmd.c_str());
    return (rc == -1) ? 127 : (rc >> 8);
}

static void write_file(const std::string& path, const std::string& contents) {
    std::ofstream ofs(path);
    ofs << contents;
}

static std::string slurp(const std::string& path) {
    std::ifstream ifs(path);
    std::stringstream ss;
    ss << ifs.rdbuf();
    return ss.str();
}

static size_t count_occurrences(const std::string& haystack,
                                const std::string& needle) {
    size_t n = 0;
    for (size_t pos = haystack.find(needle); pos != std::string::npos;
         pos = haystack.find(needle, pos + needle.size())) {
        ++n;
    }
    return n;
}

int main() {
    std::string exe = locate_wvmcc();
    if (exe.empty()) {
        std::cerr << "cannot locate wvmcc\n";
        return 4;
    }

    const std::string out = "errh_out.wasm";
    const std::string errFile = "errh_stderr.txt";
    int failures = 0;

    // Compile `src`, capture exit code and stderr.
    auto compile = [&](const std::string& srcPath) {
        int rc = run(exe + " -ffreestanding " + srcPath + " -o " + out +
                     " 2>" + errFile + " >/dev/null");
        return std::make_pair(rc, slurp(errFile));
    };

    auto check = [&](const std::string& label, bool cond) {
        if (!cond) {
            std::cerr << "FAIL: " << label << "\n";
            ++failures;
        }
    };

    // 1. Pointer argument for an arithmetic parameter: a named, located error
    //    and a non-zero exit — previously this compiled silently.
    write_file("errh_argmismatch.c",
               "int add(int a, int b) { return a + b; }\n"
               "int main(void) { int *p = 0; return add(p, 2); }\n");
    {
        auto [rc, err] = compile("errh_argmismatch.c");
        check("arg mismatch -> non-zero exit", rc != 0);
        check("arg mismatch -> names the callee",
              err.find("passing argument 1 of 'add'") != std::string::npos);
        check("arg mismatch -> has file:line:col",
              err.find("errh_argmismatch.c:2:") != std::string::npos);
    }

    // 2. Struct argument for a scalar parameter is rejected.
    write_file("errh_structarg.c",
               "struct P { int x; };\n"
               "int f(int v) { return v; }\n"
               "int main(void) { struct P p = {1}; return f(p); }\n");
    {
        auto [rc, err] = compile("errh_structarg.c");
        check("struct arg -> non-zero exit", rc != 0);
        check("struct arg -> incompatible-type error",
              err.find("has incompatible type") != std::string::npos);
    }

    // 3. One root cause, one error: an undeclared identifier in call position
    //    must not additionally produce the secondary "indirect call: unable to
    //    determine callee type" cascade.
    write_file("errh_cascade.c",
               "int main(void) { return no_such_fn(1); }\n");
    {
        auto [rc, err] = compile("errh_cascade.c");
        check("undeclared call -> non-zero exit", rc != 0);
        check("undeclared call -> exactly one error",
              count_occurrences(err, "error:") == 1);
        check("undeclared call -> no cascade",
              err.find("indirect call") == std::string::npos);
    }

    // 4. Error recovery: independent errors in two functions are both
    //    reported — the first does not abort the TU.
    write_file("errh_recovery.c",
               "int f(void) { break; return 0; }\n"
               "int g(void) { continue; return 0; }\n"
               "int main(void) { return 0; }\n");
    {
        auto [rc, err] = compile("errh_recovery.c");
        check("recovery -> non-zero exit", rc != 0);
        check("recovery -> first function's error reported",
              err.find("'break' not inside loop or switch") != std::string::npos);
        check("recovery -> second function's error reported",
              err.find("'continue' not inside a loop") != std::string::npos);
        // #27: codegen diagnostics carry source locations.
        check("recovery -> errors are located",
              err.find("errh_recovery.c:1:") != std::string::npos &&
              err.find("errh_recovery.c:2:") != std::string::npos);
    }

    // 5. Valid implicit conversions must not be rejected.
    write_file("errh_valid.c",
               "int takes_ptr(char *s) { return s ? 1 : 0; }\n"
               "int takes_bool(_Bool b) { return b; }\n"
               "long takes_long(long v) { return (int)v; }\n"
               "int main(void) {\n"
               "    char buf[4]; int *ip = 0;\n"
               "    takes_ptr(0);\n"
               "    takes_ptr(buf);\n"
               "    takes_ptr(\"lit\");\n"
               "    takes_bool(ip);\n"
               "    takes_long(42);\n"
               "    return 0;\n"
               "}\n");
    {
        auto [rc, err] = compile("errh_valid.c");
        check("valid conversions -> zero exit", rc == 0);
        check("valid conversions -> no errors",
              count_occurrences(err, "error:") == 0);
    }

    for (const char* f : {"errh_argmismatch.c", "errh_structarg.c",
                          "errh_cascade.c", "errh_recovery.c",
                          "errh_valid.c"}) {
        std::remove(f);
    }
    std::remove(out.c_str());
    std::remove(errFile.c_str());

    if (failures == 0) {
        std::cout << "all error-handling tests passed\n";
    }
    return failures == 0 ? 0 : 1;
}
