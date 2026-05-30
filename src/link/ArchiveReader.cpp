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

} // namespace wvmcc::link
