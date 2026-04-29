#pragma once
#include <optional>
#include <WasmVM.hpp>

// Helpers for inspecting WasmInstr in unit tests.
// WasmInstr is now a struct{opcode, imm} rather than a variant, so the old
// std::get_if<Instr::X> pattern no longer works.  These wrappers restore a
// similar optional-pointer style: `auto c = asI32Const(i); if (!c || c->value != 42)`

namespace instrcheck {

inline bool is(const WasmVM::WasmInstr& i, WasmVM::Opcode::Opcode op) {
    return i.opcode == op;
}

inline std::optional<WasmVM::WasmInstr::OneIdx>
asOneIdx(const WasmVM::WasmInstr& i, WasmVM::Opcode::Opcode op) {
    if (i.opcode != op) return std::nullopt;
    return std::get<WasmVM::WasmInstr::OneIdx>(i.imm);
}

inline std::optional<WasmVM::WasmInstr::ConstI32>
asI32Const(const WasmVM::WasmInstr& i) {
    if (i.opcode != WasmVM::Opcode::I32_const) return std::nullopt;
    return std::get<WasmVM::WasmInstr::ConstI32>(i.imm);
}

inline std::optional<WasmVM::WasmInstr::ConstI64>
asI64Const(const WasmVM::WasmInstr& i) {
    if (i.opcode != WasmVM::Opcode::I64_const) return std::nullopt;
    return std::get<WasmVM::WasmInstr::ConstI64>(i.imm);
}

} // namespace instrcheck
