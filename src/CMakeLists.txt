add_subdirectory(lib/PreProcessor)

add_library(cc
  lib/Compiler.cpp
)

add_executable(wasmvm-cc
  exec/wasmvm-cc.cpp
  exec/CommandParser.cpp
  lib/Linker.cpp
  lib/Archive.cpp
)
target_link_libraries(wasmvm-cc
  WasmVM
  cc
  pp
  antlr4-runtime
)
target_compile_definitions(wasmvm-cc PUBLIC
  DEFAULT_LOOKUP_PATH="${DEFAULT_LOOKUP_PATH}"
  WVMCC_VERSION="${CMAKE_PROJECT_VERSION}"
)