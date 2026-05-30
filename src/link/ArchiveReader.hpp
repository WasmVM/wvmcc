#pragma once

#include <WasmVM.hpp>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace wvmcc::link {

// Reader for a wasmvm-ar ("VMAR") archive. Loads the whole file into memory,
// parses the member index up front, and decodes each member's WasmModule
// lazily on first access (cached thereafter). Used by the lazy-pull linker
// phase (M2-L4): members are decoded only when a candidate is considered for
// inclusion.
//
// On-disk format (little-endian; see WasmVM/src/exec/Archive.cpp):
//   "VMAR" (4 bytes) | version (4 bytes)
//   paths section: uint64 paths_size | uint32 member_count
//                  member_count × { uint32 name_len | name | uint64 addr }
//   module section: at each member addr: uint64 module_size | wasm bytes
class ArchiveReader {
public:
    explicit ArchiveReader(const std::string& path);

    bool ok() const { return ok_; }
    const std::string& error() const { return err_; }
    const std::string& path() const { return path_; }

    size_t memberCount() const { return names_.size(); }
    const std::string& memberName(size_t i) const { return names_[i]; }

    // Decode member `i` to a WasmModule, caching the result. Returns nullptr
    // if the member bytes fail to decode (sets error()).
    const WasmVM::WasmModule* module(size_t i);

private:
    std::string path_;
    std::vector<char> bytes_;            // whole archive file
    std::vector<std::string> names_;     // member names, archive order
    std::vector<uint64_t> dataOffset_;   // file offset of each member's size word
    std::vector<std::optional<WasmVM::WasmModule>> cache_;
    bool ok_ = false;
    std::string err_;
};

} // namespace wvmcc::link
