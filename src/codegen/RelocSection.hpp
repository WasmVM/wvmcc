#pragma once

#include "ModuleCodegen.hpp"
#include <ostream>

namespace wvmcc::codegen {

// Append a `linking` custom section and a `reloc.CODE` custom section to
// the encoded wasm binary, using the relocations and data symbols collected
// during ModuleCodegen.
//
// Format (LEB128-encoded unless noted; subset of the LLVM wasm object
// convention — instruction-index offsets, not byte offsets, until the
// linker's encoder lands and we can compute the latter):
//
//   linking:
//     version : u32           = 2 (LLVM convention)
//     count   : u32           = number of data symbols
//     symbols : { u32 addr ; u32 name_len ; bytes name }[count]
//
//   reloc.CODE:
//     section_index : u32     = 0 (only one code section)
//     count         : u32     = number of relocations
//     entries :
//       func_idx   : u32      (index into module.funcs)
//       instr_idx  : u32      (index into that function's body)
//       symbol_idx : u32      (index into linking.symbols)
//       addend     : i64 (signed LEB)
//
// Caller writes these AFTER WasmVM::module_encode() so the resulting file
// has a valid base module followed by the custom sections.
void appendRelocSections(std::ostream& os, const ModuleCodegen& cg);

} // namespace wvmcc::codegen
