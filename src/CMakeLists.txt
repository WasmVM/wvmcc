add_executable(wvmcc
  exec/wvmcc.cpp
  lib/CommandParser.cpp
  lib/Linker.cpp
)
target_link_libraries(wvmcc
  WasmVM
)