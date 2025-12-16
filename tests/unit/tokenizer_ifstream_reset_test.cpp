#include "../../src/pp/Tokenizer.hpp"
#include <fstream>
#include <iostream>

// Verify seek-aware reset works for std::ifstream
int main() {
    using namespace wvmcc;
    const char* fname = "temp_tokenizer_reset.c";
    {
        std::ofstream ofs(fname);
        ofs << "int x;\n";
    }
    std::ifstream ifs(fname);
    Tokenizer tz(ifs);
    size_t count1 = 0;
    while (auto t = tz.next()) ++count1;
    tz.reset();
    size_t count2 = 0;
    while (auto t = tz.next()) ++count2;
    std::remove(fname);
    if (count1 == 0 || count2 == 0) {
        std::cerr << "tokenizer_ifstream_reset_test: no tokens produced\n";
        return 1;
    }
    if (count1 != count2) {
        std::cerr << "tokenizer_ifstream_reset_test: token counts differ after reset\n";
        return 2;
    }
    return 0;
}
