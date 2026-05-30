// M2-H: smoke test that the new CLI flags are recognized without error.
//
// These flags are passive in M2-H (no behavior change yet for `-isystem`,
// `-L`, `-l`, `-c`, `-nostdlib`, `-ffreestanding`). Later milestones
// (M2-I, M2-D, etc.) attach semantics. This test just locks in that they
// parse.
#include <array>
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

int main() {
    std::string exe = locate_wvmcc();
    if (exe.empty()) {
        std::cerr << "cannot locate wvmcc\n";
        return 4;
    }

    const std::string src = "cli_flags_input.c";
    {
        std::ofstream ofs(src);
        ofs << "int main(void) { return 0; }\n";
    }

    const std::string out = "cli_flags_out.wasm";
    int failures = 0;

    // -ffreestanding: parses, compiles cleanly.
    if (run(exe + " -ffreestanding " + src + " -o " + out + " 2>&1 >/dev/null") != 0) {
        std::cerr << "FAIL: -ffreestanding rejected\n"; ++failures;
    }

    // -isystem path: parses (preprocessor doesn't search it yet — M2-I).
    if (run(exe + " -isystem /tmp " + src + " -o " + out + " 2>&1 >/dev/null") != 0) {
        std::cerr << "FAIL: -isystem rejected\n"; ++failures;
    }

    // -L attached + -L detached forms parse.
    if (run(exe + " -L/tmp " + src + " -o " + out + " 2>&1 >/dev/null") != 0) {
        std::cerr << "FAIL: -L<dir> rejected\n"; ++failures;
    }
    if (run(exe + " -L /tmp " + src + " -o " + out + " 2>&1 >/dev/null") != 0) {
        std::cerr << "FAIL: -L <dir> rejected\n"; ++failures;
    }

    // -nostdlib parses.
    if (run(exe + " -nostdlib " + src + " -o " + out + " 2>&1 >/dev/null") != 0) {
        std::cerr << "FAIL: -nostdlib rejected\n"; ++failures;
    }

    // -c parses.
    if (run(exe + " -c " + src + " -o " + out + " 2>&1 >/dev/null") != 0) {
        std::cerr << "FAIL: -c rejected\n"; ++failures;
    }

    // -lc without -c should error (link step not yet implemented).
    if (run(exe + " -lc " + src + " -o " + out + " 2>/dev/null") == 0) {
        std::cerr << "FAIL: -lc without -c should error (linker not yet implemented)\n";
        ++failures;
    }

    // -lc with -c parses cleanly (library is collected, no link runs).
    if (run(exe + " -c -lc " + src + " -o " + out + " 2>&1 >/dev/null") != 0) {
        std::cerr << "FAIL: -c -lc rejected\n"; ++failures;
    }

    std::remove(src.c_str());
    std::remove(out.c_str());

    if (failures == 0) {
        std::cout << "all cli-flag tests passed\n";
    }
    return failures == 0 ? 0 : 1;
}
