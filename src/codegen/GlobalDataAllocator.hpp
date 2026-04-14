#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <WasmVM.hpp>

namespace wvmcc::codegen {

class GlobalDataAllocator {
public:
    // Allocate space for data with specified size and alignment
    size_t allocate(size_t size, size_t align);
    
    // Intern a string literal and return its address
    size_t internString(const std::string& str);
    
    // Get the current top of the data segment
    size_t currentTop() const { return currentTop_; }
    
    // Get all interned strings
    const std::vector<std::string>& getStringLiterals() const { return stringLiterals_; }
    
    // Get data segments for emission
    std::vector<WasmVM::WasmData> getDataSegments() const;
    
private:
    // Current top of the data segment
    size_t currentTop_ = 8; // Start after reserved null pointer sentinel
    
    // String literals that have been interned
    std::vector<std::string> stringLiterals_;
    
    // Map from string to its address
    std::unordered_map<std::string, size_t> stringAddresses_;
};

} // namespace wvmcc::codegen