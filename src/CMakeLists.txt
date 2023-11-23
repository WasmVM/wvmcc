add_executable(wasmvm-cc
  exec/wasmvm-cc.cpp
  lib/CommandParser.cpp
  lib/Linker.cpp
  lib/Archive.cpp
)
target_link_libraries(wasmvm-cc
  WasmVM
)
target_compile_definitions(wasmvm-cc PUBLIC DEFAULT_MODULE_PATH="${DEFAULT_MODULE_PATH}")