add_library(cc
  lib/Compiler.cpp
  lib/SourceStream.cpp
  lib/PreProcessor/PreProcessor.cpp
  lib/PreProcessor/Scanner.cpp
  lib/PreProcessor/Macro.cpp
  lib/PreProcessor/Expression.cpp
  lib/Token.cpp
  lib/Lexer.cpp
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
)
target_compile_definitions(wasmvm-cc PUBLIC
  DEFAULT_LOOKUP_PATH="${DEFAULT_LOOKUP_PATH}"
  WVMCC_VERSION="${CMAKE_PROJECT_VERSION}"
)