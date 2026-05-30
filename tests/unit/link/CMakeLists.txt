set(LINK_SRCS
    ${SRC_ROOT}/link/Linker.cpp
    ${SRC_ROOT}/link/ModuleMerge.cpp
    ${SRC_ROOT}/link/Crt0Synth.cpp
    ${SRC_ROOT}/link/SymbolResolver.cpp
    ${SRC_ROOT}/link/LinkDiagnostics.cpp
    ${SRC_ROOT}/link/RelocApply.cpp
    ${SRC_ROOT}/link/DeadCodeEliminator.cpp
    ${SRC_ROOT}/link/MapWriter.cpp
    ${SRC_ROOT}/link/ArchiveReader.cpp
    ${SRC_ROOT}/codegen/StartWrapper.cpp
)

add_executable(wvmcc-linker-skeleton-test linker_skeleton_test.cpp ${LINK_SRCS})
target_include_directories(wvmcc-linker-skeleton-test PRIVATE
    ${SRC_ROOT} ${WASMVM_INCLUDE_DIR})
target_link_directories(wvmcc-linker-skeleton-test PRIVATE ${WASMVM_LIBRARY_DIR})
target_link_libraries(wvmcc-linker-skeleton-test PRIVATE ${WASMVM_LIBRARY})
add_test(NAME linker-skeleton COMMAND wvmcc-linker-skeleton-test)

add_executable(wvmcc-module-merge-test module_merge_test.cpp ${LINK_SRCS})
target_include_directories(wvmcc-module-merge-test PRIVATE
    ${SRC_ROOT} ${WASMVM_INCLUDE_DIR})
target_link_directories(wvmcc-module-merge-test PRIVATE ${WASMVM_LIBRARY_DIR})
target_link_libraries(wvmcc-module-merge-test PRIVATE ${WASMVM_LIBRARY})
add_test(NAME module-merge COMMAND wvmcc-module-merge-test)

add_executable(wvmcc-symbol-resolver-test symbol_resolver_test.cpp ${LINK_SRCS})
target_include_directories(wvmcc-symbol-resolver-test PRIVATE
    ${SRC_ROOT} ${WASMVM_INCLUDE_DIR})
target_link_directories(wvmcc-symbol-resolver-test PRIVATE ${WASMVM_LIBRARY_DIR})
target_link_libraries(wvmcc-symbol-resolver-test PRIVATE ${WASMVM_LIBRARY})
add_test(NAME symbol-resolver COMMAND wvmcc-symbol-resolver-test)

add_executable(wvmcc-dce-test dce_test.cpp ${LINK_SRCS})
target_include_directories(wvmcc-dce-test PRIVATE
    ${SRC_ROOT} ${WASMVM_INCLUDE_DIR})
target_link_directories(wvmcc-dce-test PRIVATE ${WASMVM_LIBRARY_DIR})
target_link_libraries(wvmcc-dce-test PRIVATE ${WASMVM_LIBRARY})
add_test(NAME dce COMMAND wvmcc-dce-test)
