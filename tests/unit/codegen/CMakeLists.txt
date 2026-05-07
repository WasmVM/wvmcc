set(SRC ${PROJECT_SOURCE_DIR}/src)

set(CODEGEN_SRCS
    ${SRC}/codegen/FunctionCodegen.cpp
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
