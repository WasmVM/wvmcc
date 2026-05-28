set(LINK_SRCS
    ${SRC_ROOT}/link/Linker.cpp
)

add_executable(wvmcc-linker-skeleton-test linker_skeleton_test.cpp ${LINK_SRCS})
target_include_directories(wvmcc-linker-skeleton-test PRIVATE
    ${SRC_ROOT} ${WASMVM_INCLUDE_DIR})
target_link_directories(wvmcc-linker-skeleton-test PRIVATE ${WASMVM_LIBRARY_DIR})
target_link_libraries(wvmcc-linker-skeleton-test PRIVATE ${WASMVM_LIBRARY})
add_test(NAME linker-skeleton COMMAND wvmcc-linker-skeleton-test)
