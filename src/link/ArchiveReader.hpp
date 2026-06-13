#pragma once

#include "Linker.hpp"   // DataPtrSite
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

    // Data-pointer relocation sites (M2-L8) for member `i`, parsed from its
    // `reloc.CODE` custom section (which module_decode drops). Empty if the
    // member carries no relocs.
    const std::vector<DataPtrSite>& memberRelocs(size_t i);

    // #79: function-pointer relocation sites for member `i`, parsed from its
    // `reloc.FUNCPTR` custom section. Empty if the member carries none.
    const std::vector<DataPtrSite>& memberFuncPtrRelocs(size_t i);

private:
    // Parse a `reloc.*` custom section ("reloc.CODE" or "reloc.FUNCPTR") of
    // member `i` into `cache`, returning the (funcIdx, instrIdx) sites.
    const std::vector<DataPtrSite>& parseRelocSection(
        size_t i, const std::string& sectionName, bool hasSymAndAddend,
        std::vector<std::optional<std::vector<DataPtrSite>>>& cache);

    std::string path_;
    std::vector<char> bytes_;            // whole archive file
    std::vector<std::string> names_;     // member names, archive order
    std::vector<uint64_t> dataOffset_;   // file offset of each member's size word
    std::vector<std::optional<WasmVM::WasmModule>> cache_;
    std::vector<std::optional<std::vector<DataPtrSite>>> relocCache_;
    std::vector<std::optional<std::vector<DataPtrSite>>> funcPtrRelocCache_;
    bool ok_ = false;
    std::string err_;
};

// Load a standalone linkable object (a `-c` artifact: a wasm module with
// `reloc.CODE` / `reloc.FUNCPTR` custom sections appended) from disk into a
// LinkInput::InMemoryModule, mirroring what the driver builds for a freshly
// compiled user TU. `origin` is stored for diagnostics. On failure returns
// nullopt and sets `err`.
//
// This is the on-disk counterpart of the compile path: it lets object inputs
// (.o / linkable .wasm) named on the command line be linked instead of being
// re-parsed as C source.
std::optional<LinkInput::InMemoryModule> loadObjectFile(const std::string& path,
                                                        std::string& err);

} // namespace wvmcc::link
