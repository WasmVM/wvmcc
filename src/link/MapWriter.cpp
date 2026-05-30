#include "MapWriter.hpp"

#include <cstdint>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <variant>

namespace wvmcc::link::map {

namespace {

uint64_t readConstAddr(const WasmVM::ConstInstr& c) {
    uint64_t v = 0;
    std::visit([&](const auto& vv) {
        using T = std::decay_t<decltype(vv)>;
        if constexpr (std::is_same_v<T, WasmVM::Instr::I64_const>) {
            v = (uint64_t)vv.value;
        } else if constexpr (std::is_same_v<T, WasmVM::Instr::I32_const>) {
            v = (uint64_t)vv.value;
        }
    }, c);
    return v;
}

const char* importKindName(const WasmVM::WasmImport& imp) {
    if (std::holds_alternative<WasmVM::index_t>(imp.desc))    return "func";
    if (std::holds_alternative<WasmVM::TableType>(imp.desc))  return "table";
    if (std::holds_alternative<WasmVM::MemType>(imp.desc))    return "memory";
    if (std::holds_alternative<WasmVM::GlobalType>(imp.desc)) return "global";
    return "?";
}

const char* exportKindName(WasmVM::WasmExport::DescType d) {
    switch (d) {
        case WasmVM::WasmExport::DescType::func:   return "func";
        case WasmVM::WasmExport::DescType::table:  return "table";
        case WasmVM::WasmExport::DescType::mem:    return "memory";
        case WasmVM::WasmExport::DescType::global: return "global";
    }
    return "?";
}

} // namespace

void writeMap(LinkContext& ctx) {
    if (ctx.opts.map_path.empty()) return;

    std::ofstream out(ctx.opts.map_path);
    if (!out) {
        ctx.error("link: cannot open --map output file '" + ctx.opts.map_path + "'");
        return;
    }

    const auto& m = ctx.output;

    out << "wvmcc linker map\n";
    out << "================\n\n";

    out << "Inputs:\n";
    for (const auto& in : ctx.inputs) {
        if (auto* mm = std::get_if<LinkInput::InMemoryModule>(&in.source)) {
            out << "  " << mm->origin << " (in-memory)\n";
        } else if (auto* ap = std::get_if<LinkInput::ArchivePath>(&in.source)) {
            out << "  " << ap->path << " (archive)\n";
        }
    }
    out << "\n";

    // Function summary.
    WasmVM::index_t funcImports = 0;
    for (const auto& imp : m.imports) {
        if (std::holds_alternative<WasmVM::index_t>(imp.desc)) ++funcImports;
    }
    out << "Functions:\n";
    out << "  imports (host runtime + libc): " << funcImports << "\n";
    out << "  defined:                      " << m.funcs.size() << "\n";
    out << "  total:                        "
        << (funcImports + m.funcs.size()) << "\n";
    if (m.start.has_value()) {
        out << "  start function index:         " << *m.start << "\n";
    }
    out << "\n";

    out << "Imports:\n";
    if (m.imports.empty()) {
        out << "  (none)\n";
    } else {
        for (const auto& imp : m.imports) {
            out << "  " << imp.module << "." << imp.name
                << " [" << importKindName(imp) << "]\n";
        }
    }
    out << "\n";

    out << "Exports:\n";
    if (m.exports.empty()) {
        out << "  (none)\n";
    } else {
        for (const auto& ex : m.exports) {
            out << "  " << ex.name
                << " [" << exportKindName(ex.desc)
                << " #" << ex.index << "]\n";
        }
    }
    out << "\n";

    out << "Data segments:\n";
    if (m.datas.empty()) {
        out << "  (none)\n";
    } else {
        for (size_t i = 0; i < m.datas.size(); ++i) {
            const auto& d = m.datas[i];
            uint64_t base = d.mode.offset.has_value()
                                ? readConstAddr(*d.mode.offset) : 0;
            uint64_t end = base + d.init.size();
            std::ostringstream ss;
            ss << "  [" << std::setw(2) << i << "] "
               << std::hex << "0x" << base << " .. 0x" << end << std::dec
               << "  (" << d.init.size() << " bytes)";
            out << ss.str() << "\n";
        }
    }
    out << "\n";

    out << "Globals:\n";
    for (size_t i = 0; i < m.globals.size(); ++i) {
        const auto& g = m.globals[i];
        const char* mut = (g.type.mut == WasmVM::GlobalType::variable) ? "mut" : "const";
        const char* ty  = (g.type.type == WasmVM::ValueType::i64) ? "i64"
                        : (g.type.type == WasmVM::ValueType::i32) ? "i32"
                        : (g.type.type == WasmVM::ValueType::f32) ? "f32"
                        : (g.type.type == WasmVM::ValueType::f64) ? "f64" : "?";
        out << "  global " << i << ": " << mut << " " << ty
            << " = (init expr)\n";
    }
    out.flush();
}

} // namespace wvmcc::link::map
