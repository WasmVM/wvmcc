set(SRC ${PROJECT_SOURCE_DIR}/src)

set(CODEGEN_SRCS
    ${SRC}/codegen/FunctionCodegen.cpp
    ${SRC}/codegen/ModuleCodegen.cpp
    ${SRC}/codegen/StartWrapper.cpp
    ${SRC}/codegen/RelocSection.cpp
    ${SRC}/codegen/TypeMap.cpp
    ${SRC}/codegen/SymbolTable.cpp
    ${SRC}/codegen/LayoutEngine.cpp
    ${SRC}/codegen/AddressTakenAnalyzer.cpp
    ${SRC}/codegen/GlobalDataAllocator.cpp
    ${SRC}/codegen/TypeIndexCache.cpp
    ${SRC}/parser/ASTVisitor.cpp
    ${SRC}/parser/Semantic.cpp
    ${SRC}/parser/ConstExprEval.cpp
    ${SRC}/parser/ASTPrinter.cpp
)

set(CODEGEN_FRONTEND_SRCS ${CODEGEN_SRCS}
    ${SRC}/parser/Lexer.cpp
    ${SRC}/parser/Parser.cpp
    ${SRC}/pp/Preprocessor.cpp
    ${SRC}/pp/Tokenizer.cpp
    ${SRC}/pp/MacroTable.cpp
    ${SRC}/pp/ConstExprParser.cpp
)

add_executable(wvmcc-basic-codegen-test basic_codegen_test.cpp ${CODEGEN_SRCS})
target_include_directories(wvmcc-basic-codegen-test PRIVATE ${WASMVM_INCLUDE_DIR})
target_link_directories(wvmcc-basic-codegen-test PRIVATE ${WASMVM_LIBRARY_DIR})
target_link_libraries(wvmcc-basic-codegen-test PRIVATE ${WASMVM_LIBRARY})
add_test(NAME basic-codegen COMMAND wvmcc-basic-codegen-test)

add_executable(wvmcc-expression-emission-test expression_emission_test.cpp ${CODEGEN_SRCS})
target_include_directories(wvmcc-expression-emission-test PRIVATE ${WASMVM_INCLUDE_DIR})
target_link_directories(wvmcc-expression-emission-test PRIVATE ${WASMVM_LIBRARY_DIR})
target_link_libraries(wvmcc-expression-emission-test PRIVATE ${WASMVM_LIBRARY})
add_test(NAME expression-emission COMMAND wvmcc-expression-emission-test)

add_executable(wvmcc-statement-emission-test statement_emission_test.cpp ${CODEGEN_SRCS})
target_include_directories(wvmcc-statement-emission-test PRIVATE ${WASMVM_INCLUDE_DIR})
target_link_directories(wvmcc-statement-emission-test PRIVATE ${WASMVM_LIBRARY_DIR})
target_link_libraries(wvmcc-statement-emission-test PRIVATE ${WASMVM_LIBRARY})
add_test(NAME statement-emission COMMAND wvmcc-statement-emission-test)

add_executable(wvmcc-control-flow-test control_flow_test.cpp ${CODEGEN_SRCS})
target_include_directories(wvmcc-control-flow-test PRIVATE ${WASMVM_INCLUDE_DIR})
target_link_directories(wvmcc-control-flow-test PRIVATE ${WASMVM_LIBRARY_DIR})
target_link_libraries(wvmcc-control-flow-test PRIVATE ${WASMVM_LIBRARY})
add_test(NAME control-flow COMMAND wvmcc-control-flow-test)

add_executable(wvmcc-phase4-test phase4_test.cpp ${CODEGEN_SRCS})
target_include_directories(wvmcc-phase4-test PRIVATE ${WASMVM_INCLUDE_DIR})
target_link_directories(wvmcc-phase4-test PRIVATE ${WASMVM_LIBRARY_DIR})
target_link_libraries(wvmcc-phase4-test PRIVATE ${WASMVM_LIBRARY})
add_test(NAME phase4 COMMAND wvmcc-phase4-test)

add_executable(wvmcc-import-attribute-test import_attribute_test.cpp ${CODEGEN_FRONTEND_SRCS})
target_include_directories(wvmcc-import-attribute-test PRIVATE ${WASMVM_INCLUDE_DIR})
target_link_directories(wvmcc-import-attribute-test PRIVATE ${WASMVM_LIBRARY_DIR})
target_link_libraries(wvmcc-import-attribute-test PRIVATE ${WASMVM_LIBRARY})
add_test(NAME import-attribute COMMAND wvmcc-import-attribute-test)

add_executable(wvmcc-start-wrapper-test start_wrapper_test.cpp ${CODEGEN_FRONTEND_SRCS})
target_include_directories(wvmcc-start-wrapper-test PRIVATE ${WASMVM_INCLUDE_DIR})
target_link_directories(wvmcc-start-wrapper-test PRIVATE ${WASMVM_LIBRARY_DIR})
target_link_libraries(wvmcc-start-wrapper-test PRIVATE ${WASMVM_LIBRARY})
add_test(NAME start-wrapper COMMAND wvmcc-start-wrapper-test)

add_executable(wvmcc-linkable-mode-test linkable_mode_test.cpp ${CODEGEN_FRONTEND_SRCS})
target_include_directories(wvmcc-linkable-mode-test PRIVATE ${WASMVM_INCLUDE_DIR})
target_link_directories(wvmcc-linkable-mode-test PRIVATE ${WASMVM_LIBRARY_DIR})
target_link_libraries(wvmcc-linkable-mode-test PRIVATE ${WASMVM_LIBRARY})
add_test(NAME linkable-mode COMMAND wvmcc-linkable-mode-test)

add_executable(wvmcc-export-visibility-test export_visibility_test.cpp ${CODEGEN_FRONTEND_SRCS})
target_include_directories(wvmcc-export-visibility-test PRIVATE ${WASMVM_INCLUDE_DIR})
target_link_directories(wvmcc-export-visibility-test PRIVATE ${WASMVM_LIBRARY_DIR})
target_link_libraries(wvmcc-export-visibility-test PRIVATE ${WASMVM_LIBRARY})
add_test(NAME export-visibility COMMAND wvmcc-export-visibility-test)

add_executable(wvmcc-reloc-section-test reloc_section_test.cpp ${CODEGEN_FRONTEND_SRCS})
target_include_directories(wvmcc-reloc-section-test PRIVATE ${WASMVM_INCLUDE_DIR})
target_link_directories(wvmcc-reloc-section-test PRIVATE ${WASMVM_LIBRARY_DIR})
target_link_libraries(wvmcc-reloc-section-test PRIVATE ${WASMVM_LIBRARY})
add_test(NAME reloc-section COMMAND wvmcc-reloc-section-test)
