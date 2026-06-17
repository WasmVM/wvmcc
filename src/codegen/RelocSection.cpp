#include "RelocSection.hpp"

#include <cstdint>
#include <cstring>
#include <ostream>
#include <string>
#include <vector>

namespace wvmcc::codegen {

namespace {

// LEB128 helpers — sufficient for the small payloads in linking / reloc.CODE.
void writeU(std::vector<uint8_t>& out, uint64_t v) {
    do {
        uint8_t b = (uint8_t)(v & 0x7f);
        v >>= 7;
        if (v) b |= 0x80;
        out.push_back(b);
    } while (v);
}

void writeS(std::vector<uint8_t>& out, int64_t v) {
    bool more = true;
    while (more) {
        uint8_t b = (uint8_t)(v & 0x7f);
        v >>= 7; // arithmetic shift (sign-extends in C++20 for signed types)
        bool signBit = (b & 0x40) != 0;
        if ((v == 0 && !signBit) || (v == -1 && signBit)) {
            more = false;
        } else {
            b |= 0x80;
        }
        out.push_back(b);
    }
}

void writeBytes(std::vector<uint8_t>& out, const std::string& s) {
    writeU(out, s.size());
    out.insert(out.end(), s.begin(), s.end());
}

// Emit a custom section to `os`.
void emitCustomSection(std::ostream& os, const std::string& name,
                       const std::vector<uint8_t>& body) {
    // section_id = 0 (custom)
    char id = 0;
    os.write(&id, 1);
    // payload = name (LEB-prefixed) + body
    std::vector<uint8_t> payload;
    writeBytes(payload, name);
    payload.insert(payload.end(), body.begin(), body.end());
    // size: LEB
    std::vector<uint8_t> sizeBytes;
    writeU(sizeBytes, payload.size());
    os.write(reinterpret_cast<const char*>(sizeBytes.data()),
             (std::streamsize)sizeBytes.size());
    os.write(reinterpret_cast<const char*>(payload.data()),
             (std::streamsize)payload.size());
}

} // namespace

// NOTE (LANG-6.6-06): address-constant pointers baked into data segments
// (ModuleCodegen::getDataSegDataRelocs / getDataSegFuncPtrRelocs) are NOT yet
// serialized here. The single-invocation driver path carries them straight to
// the linker via LinkInput::InMemoryModule, so `wvmcc a.c b.c -lc -o out` and
// the standard suite work. Separately-compiled objects (`-c` then link from
// disk/archive) would need reloc.DATAPTR / reloc.DATAFP custom sections plus
// matching ArchiveReader parsing; deferred until a `-c` workflow exercises
// file-scope address-constant initializers (libc currently uses none).
void appendRelocSections(std::ostream& os, const ModuleCodegen& cg) {
    const auto& syms  = cg.getDataSymbols();
    const auto& relocs = cg.getRelocations();

    // Skip entirely when there's nothing to record — keeps default-mode
    // hello-world modules from accumulating empty sections.
    if (syms.empty() && relocs.empty() && cg.getFuncPtrRelocs().empty()) return;

    // --- linking section ---
    {
        std::vector<uint8_t> body;
        writeU(body, 2);                  // version (LLVM convention)
        writeU(body, syms.size());
        for (const auto& s : syms) {
            writeU(body, s.address);
            writeBytes(body, s.name);
        }
        emitCustomSection(os, "linking", body);
    }

    // --- reloc.CODE section ---
    {
        std::vector<uint8_t> body;
        writeU(body, 0);                  // section_index = 0
        writeU(body, relocs.size());
        for (const auto& r : relocs) {
            writeU(body, r.codeFuncIdx);
            writeU(body, r.instrIdx);
            writeU(body, r.dataSymbolIdx);
            writeS(body, r.addend);
        }
        emitCustomSection(os, "reloc.CODE", body);
    }

    // --- reloc.FUNCPTR section (#79) ---
    // Function-pointer i64.const sites whose embedded funcref-table slot the
    // linker rebases when merging per-TU tables. Just (funcIdx, instrIdx) pairs.
    {
        const auto& fpRelocs = cg.getFuncPtrRelocs();
        if (!fpRelocs.empty()) {
            std::vector<uint8_t> body;
            writeU(body, 0);              // section_index = 0
            writeU(body, fpRelocs.size());
            for (const auto& r : fpRelocs) {
                writeU(body, r.codeFuncIdx);
                writeU(body, r.instrIdx);
            }
            emitCustomSection(os, "reloc.FUNCPTR", body);
        }
    }
}

} // namespace wvmcc::codegen
