#include <cstdint>
#include "ArchiveReader.hpp"

#include <cstring>
#include <fstream>
#include <sstream>

namespace wvmcc::link {

namespace {

// Read a little-endian integer of type T from `buf` at `off`, advancing `off`.
// Returns false if it would read past `size`.
template <typename T>
bool readLE(const char* buf, size_t size, size_t& off, T& out) {
    if (off + sizeof(T) > size) return false;
    std::memcpy(&out, buf + off, sizeof(T));
    off += sizeof(T);
    return true;
}

} // namespace

ArchiveReader::ArchiveReader(const std::string& path) : path_(path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) {
        err_ = "cannot open archive: " + path;
        return;
    }
    bytes_.assign(std::istreambuf_iterator<char>(f),
                  std::istreambuf_iterator<char>());
    const char* buf = bytes_.data();
    const size_t size = bytes_.size();

    size_t off = 0;
    // Header: 4 magic bytes + 4 version bytes.
    if (size < 8 || std::memcmp(buf, "VMAR", 4) != 0) {
        err_ = "not a VMAR archive: " + path;
        return;
    }
    off = 8;

    uint64_t pathsSize = 0;   // unused (we index by member addresses)
    uint32_t count = 0;
    if (!readLE(buf, size, off, pathsSize) || !readLE(buf, size, off, count)) {
        err_ = "truncated archive header: " + path;
        return;
    }

    names_.reserve(count);
    dataOffset_.reserve(count);
    for (uint32_t i = 0; i < count; ++i) {
        uint32_t nameLen = 0;
        if (!readLE(buf, size, off, nameLen)) {
            err_ = "truncated archive index: " + path;
            return;
        }
        if (off + nameLen > size) {
            err_ = "truncated member name: " + path;
            return;
        }
        std::string name(buf + off, nameLen);
        off += nameLen;
        uint64_t addr = 0;
        if (!readLE(buf, size, off, addr)) {
            err_ = "truncated member address: " + path;
            return;
        }
        if (addr + sizeof(uint64_t) > size) {
            err_ = "member address out of range: " + path;
            return;
        }
        names_.push_back(std::move(name));
        dataOffset_.push_back(addr);
    }

    cache_.resize(names_.size());
    relocCache_.resize(names_.size());
    funcPtrRelocCache_.resize(names_.size());
    ok_ = true;
}

const WasmVM::WasmModule* ArchiveReader::module(size_t i) {
    if (i >= names_.size()) return nullptr;
    if (cache_[i].has_value()) return &*cache_[i];

    const char* buf = bytes_.data();
    const size_t size = bytes_.size();
    size_t off = dataOffset_[i];
    uint64_t modSize = 0;
    if (!readLE(buf, size, off, modSize) || off + modSize > size) {
        err_ = "truncated member body: " + names_[i];
        return nullptr;
    }
    std::string body(buf + off, (size_t)modSize);
    std::istringstream in(body, std::ios::binary);
    try {
        cache_[i] = WasmVM::module_decode(in);
    } catch (const std::exception& e) {
        err_ = "decode failed for member '" + names_[i] + "': " + e.what();
        return nullptr;
    } catch (...) {
        err_ = "decode failed for member '" + names_[i] + "'";
        return nullptr;
    }
    return &*cache_[i];
}

namespace {

// Unsigned LEB128 decode from buf[pos..end), advancing pos. Returns false on
// truncation or overrun.
bool readULEB(const char* buf, size_t end, size_t& pos, uint64_t& out) {
    out = 0;
    int shift = 0;
    while (pos < end) {
        uint8_t b = (uint8_t)buf[pos++];
        if (shift < 64) out |= (uint64_t)(b & 0x7f) << shift;
        shift += 7;
        if (!(b & 0x80)) return true;
        if (shift >= 70) return false;
    }
    return false;
}

// Walk the custom sections of the wasm module occupying buf[modStart, modEnd)
// and collect the (funcIdx, instrIdx) relocation sites recorded in the named
// `reloc.*` section. `hasSymAndAddend` consumes the trailing symbol-index and
// addend fields (present in reloc.CODE, absent in reloc.FUNCPTR). Shared by the
// archive-member path (ArchiveReader::parseRelocSection) and the standalone
// object path (loadObjectFile) so both decode relocs identically.
std::vector<DataPtrSite> parseRelocSites(const char* buf, size_t modStart,
                                         size_t modEnd,
                                         const std::string& sectionName,
                                         bool hasSymAndAddend) {
    std::vector<DataPtrSite> out;
    if (modEnd < modStart + 8) return out;
    // Skip the 8-byte wasm header (magic + version), then walk sections.
    size_t pos = modStart + 8;
    while (pos < modEnd) {
        uint8_t id = (uint8_t)buf[pos++];
        uint64_t secSize = 0;
        if (!readULEB(buf, modEnd, pos, secSize)) break;
        const size_t secStart = pos;
        const size_t secEnd = secStart + (size_t)secSize;
        if (secEnd > modEnd) break;
        if (id == 0) {
            // Custom section: name_len, name, content.
            size_t p = secStart;
            uint64_t nameLen = 0;
            if (readULEB(buf, secEnd, p, nameLen) && p + nameLen <= secEnd) {
                std::string name(buf + p, (size_t)nameLen);
                p += nameLen;
                if (name == sectionName) {
                    uint64_t secIndex = 0, count = 0;
                    if (readULEB(buf, secEnd, p, secIndex) &&
                        readULEB(buf, secEnd, p, count)) {
                        for (uint64_t k = 0; k < count; ++k) {
                            uint64_t fIdx = 0, iIdx = 0, symIdx = 0, addend = 0;
                            if (!readULEB(buf, secEnd, p, fIdx)) break;
                            if (!readULEB(buf, secEnd, p, iIdx)) break;
                            if (hasSymAndAddend) {
                                if (!readULEB(buf, secEnd, p, symIdx)) break;
                                // addend is signed LEB; not needed for uniform
                                // per-TU rebasing — just consume it.
                                if (!readULEB(buf, secEnd, p, addend)) break;
                            }
                            out.push_back({(uint32_t)fIdx, (uint32_t)iIdx});
                        }
                    }
                }
            }
        }
        pos = secEnd;
    }
    return out;
}

} // namespace

const std::vector<DataPtrSite>& ArchiveReader::memberRelocs(size_t i) {
    return parseRelocSection(i, "reloc.CODE", /*hasSymAndAddend=*/true,
                             relocCache_);
}

const std::vector<DataPtrSite>& ArchiveReader::memberFuncPtrRelocs(size_t i) {
    return parseRelocSection(i, "reloc.FUNCPTR", /*hasSymAndAddend=*/false,
                             funcPtrRelocCache_);
}

const std::vector<DataPtrSite>& ArchiveReader::parseRelocSection(
    size_t i, const std::string& sectionName, bool hasSymAndAddend,
    std::vector<std::optional<std::vector<DataPtrSite>>>& cache) {
    static const std::vector<DataPtrSite> empty;
    if (i >= names_.size()) return empty;
    if (cache[i].has_value()) return *cache[i];
    cache[i].emplace(); // default to empty; fill if we find the section
    auto& out = *cache[i];

    const char* buf = bytes_.data();
    const size_t fileSize = bytes_.size();
    // Member wasm bytes: skip the 8-byte module_size word at dataOffset_[i].
    size_t off = dataOffset_[i];
    uint64_t modSize = 0;
    if (!readLE(buf, fileSize, off, modSize)) return out;
    const size_t modStart = off;
    const size_t modEnd = modStart + (size_t)modSize;
    if (modEnd > fileSize || modSize < 8) return out;

    out = parseRelocSites(buf, modStart, modEnd, sectionName, hasSymAndAddend);
    return out;
}

std::optional<LinkInput::InMemoryModule> loadObjectFile(const std::string& path,
                                                        std::string& err) {
    std::ifstream f(path, std::ios::binary);
    if (!f) {
        err = "cannot open object file: " + path;
        return std::nullopt;
    }
    std::vector<char> bytes(std::istreambuf_iterator<char>(f),
                            std::istreambuf_iterator<char>{});

    // A linkable object is a base wasm module (which module_decode reads, then
    // stops at the trailing custom sections) followed by reloc.CODE /
    // reloc.FUNCPTR. Validate the wasm magic before decoding so a stray text
    // file named *.o produces a clear error rather than a decode throw.
    if (bytes.size() < 8 ||
        std::memcmp(bytes.data(), "\0asm", 4) != 0) {
        err = "not a wasm object (bad magic): " + path;
        return std::nullopt;
    }

    LinkInput::InMemoryModule mod;
    mod.origin = path;
    std::string body(bytes.data(), bytes.size());
    std::istringstream in(body, std::ios::binary);
    try {
        mod.module = WasmVM::module_decode(in);
    } catch (const std::exception& e) {
        err = "decode failed for object '" + path + "': " + e.what();
        return std::nullopt;
    } catch (...) {
        err = "decode failed for object '" + path + "'";
        return std::nullopt;
    }

    // Parse the reloc sections that module_decode dropped, so the merge phase
    // can rebase data pointers (reloc.CODE) and funcref-table slots
    // (reloc.FUNCPTR) exactly as it does for archive members.
    const char* buf = bytes.data();
    const size_t modStart = 0;
    const size_t modEnd = bytes.size();
    mod.dataRelocs =
        parseRelocSites(buf, modStart, modEnd, "reloc.CODE", /*hasSymAndAddend=*/true);
    mod.funcPtrRelocs = parseRelocSites(buf, modStart, modEnd, "reloc.FUNCPTR",
                                        /*hasSymAndAddend=*/false);
    return mod;
}

} // namespace wvmcc::link
