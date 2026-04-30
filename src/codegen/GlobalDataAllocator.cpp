#include "GlobalDataAllocator.hpp"
#include <cstring>

namespace wvmcc::codegen {

size_t GlobalDataAllocator::allocate(size_t size, size_t align) {
    // Align the current top to the requested alignment
    size_t alignedTop = (currentTop_ + align - 1) & ~(align - 1);
    
    // Allocate space
    size_t address = alignedTop;
    currentTop_ = alignedTop + size;
    
    return address;
}

size_t GlobalDataAllocator::internString(const std::string& str) {
    // Check if string is already interned
    auto it = stringAddresses_.find(str);
    if (it != stringAddresses_.end()) {
        return it->second;
    }
    
    // Add string to literals list
    size_t address = allocate(str.size() + 1, 1); // +1 for null terminator
    stringLiterals_.push_back(str);
    stringAddresses_[str] = address;
    
    return address;
}

std::vector<WasmVM::WasmData> GlobalDataAllocator::getDataSegments() const {
    std::vector<WasmVM::WasmData> segments;

    for (const auto& str : stringLiterals_) {
        size_t addr = stringAddresses_.at(str);

        WasmVM::WasmData data;
        data.mode.type = WasmVM::WasmData::DataMode::Mode::active;
        data.mode.memidx = 0;
        data.mode.offset = WasmVM::Instr::I64_const{(WasmVM::i64_t)addr};
        for (char c : str) {
            data.init.push_back(std::byte(c));
        }
        data.init.push_back(std::byte(0));

        segments.push_back(std::move(data));
    }

    return segments;
}

} // namespace wvmcc::codegen