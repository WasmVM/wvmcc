#include <fstream>
#include <iostream>
#include <string>
#include <cstdio>
#include <array>
#include <memory>

int main() {
    const std::string fname = "temp_preprocess_only.c";
    // Locate the built `wvmcc` executable in common relative locations
    const std::vector<std::string> candidates = {"./wvmcc", "../wvmcc", "./tests/wvmcc", "../tests/wvmcc", "../../wvmcc"};
    std::string exePath;
    for (const auto &c : candidates) {
        if (std::FILE *f = std::fopen(c.c_str(), "r")) { std::fclose(f); exePath = c; break; }
    }
    if (exePath.empty()) {
        std::cerr << "cannot locate wvmcc executable in working dirs\n";
        std::remove(fname.c_str());
        return 4;
    }

    // Run via shell so we can capture stderr as well (redirect 2>&1)
    const std::string cmd = "sh -c '" + exePath + " -E " + fname + " 2>&1'";

    // Prepare a simple input with a macro and an include-like pattern
    {
        std::ofstream ofs(fname);
        ofs << "#define X 42\n";
        ofs << "int a = X;\n";
    }

    // Build command to run the built wvmcc from tests directory
    // Note: CTest will run the test from build directory, so executable
    // is under the top-level binary directory (tests target program).
    // We'll execute relative path to the test harness's current working dir.

    // Use popen to capture stdout
    std::array<char, 256> buf;
    std::string output;
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) {
        std::remove(fname.c_str());
        std::cerr << "failed to run command\n";
        return 1;
    }
    while (fgets(buf.data(), buf.size(), pipe) != nullptr) {
        output += buf.data();
    }
    int rc = pclose(pipe);
    std::remove(fname.c_str());

    if (rc != 0) {
        std::cerr << "wvmcc -E returned non-zero, output:\n" << output << "\n";
        return 2;
    }

    // Expect the macro to be expanded in output
    if (output.find("int a = 42") == std::string::npos) {
        std::cerr << "unexpected preprocess output:\n" << output << "\n";
        return 3;
    }

    std::cout << "preprocess_only_test: OK\n";
    return 0;
}
