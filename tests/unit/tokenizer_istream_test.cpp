#include "../../src/pp/Tokenizer.hpp"
#include <sstream>
#include <vector>
#include <iostream>

// Verify Tokenizer can read from std::istringstream incrementally
int main() {
    using namespace wvmcc;
    std::istringstream iss("int main()\n{\n  return 0;\n}\n");
    Tokenizer tz(iss);
    size_t count = 0;
    while (auto tok = tz.next()) { ++count; }
    if (count == 0) {
        std::cerr << "tokenizer_istream_test: no tokens produced from stream\n";
        return 1;
    }
    return 0;
}
