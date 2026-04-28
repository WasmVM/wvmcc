#pragma once

#include <unordered_set>
#include <string>
#include <vector>
#include "../parser/AST.hpp"

namespace wvmcc::codegen {

class AddressTakenAnalyzer {
public:
    // Analyze a function definition and return set of address-taken variable names
    std::unordered_set<std::string> analyze(const wvmcc::parser::FunctionDefPtr& funcDef);

private:
    // Helper to walk the AST and find address-taken variables
    void walk(const std::vector<wvmcc::parser::BlockItemPtr>& blockItems, std::unordered_set<std::string>& addressTakenNames);
    void walk(const wvmcc::parser::BlockItemPtr& item, std::unordered_set<std::string>& addressTakenNames);
    void walk(const wvmcc::parser::StmtPtr& stmt, std::unordered_set<std::string>& addressTakenNames);
    void walk(const wvmcc::parser::ExprPtr& expr, std::unordered_set<std::string>& addressTakenNames);
};

} // namespace wvmcc::codegen