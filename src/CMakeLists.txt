add_executable(wvmcc
  exec/wvmcc.cpp
  exec/CommandParser.cpp
)
target_link_libraries(wvmcc
  WasmVM
)