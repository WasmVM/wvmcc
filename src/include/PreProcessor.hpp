#ifndef WVMCC_PreProcessor_DEF
#define WVMCC_PreProcessor_DEF

#include <filesystem>
#include <stack>
#include <SourceFile.hpp>

namespace WasmVM {

struct PreProcessor {

    PreProcessor(std::filesystem::path path);
    ~PreProcessor();

private:
    std::stack<SourceFile*> sources;
};

}

#endif