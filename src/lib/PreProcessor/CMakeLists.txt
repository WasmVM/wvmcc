find_package(Java 1.11 REQUIRED)
add_custom_command(OUTPUT PPToken.cpp
  COMMAND ${Java_JAVA_EXECUTABLE} -jar ${PROJECT_ROOT}/external/antlr4-4.13.3-dev-complete.jar -package WasmVM -no-visitor -no-listener -Xexact-output-dir -o ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_LIST_DIR}/PPToken.g4
  DEPENDS ${CMAKE_CURRENT_LIST_DIR}/PPToken.g4
)

add_library(pp
  PreProcessor.cpp
  PPToken.cpp
)
target_include_directories(pp 
  PUBLIC ${PROJECT_BINARY_DIR}/external/antlr4_runtime/src/antlr4_runtime-install/include/antlr4-runtime
  PUBLIC ${CMAKE_CURRENT_BINARY_DIR}
)
target_link_libraries(pp antlr4-runtime)

add_executable(test_pp
  PPToken.cpp
  test.cpp
)
target_include_directories(test_pp 
  PUBLIC ${PROJECT_BINARY_DIR}/external/antlr4_runtime/src/antlr4_runtime-install/include/antlr4-runtime
  PUBLIC ${CMAKE_CURRENT_BINARY_DIR}
)
target_link_libraries(test_pp pp)