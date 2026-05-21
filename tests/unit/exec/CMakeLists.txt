add_executable(wvmcc-preprocess-only-test preprocess_only_test.cpp)
add_dependencies(wvmcc-preprocess-only-test wvmcc)
add_test(NAME preprocess-only COMMAND wvmcc-preprocess-only-test)

add_executable(wvmcc-sysroot-resolve-test
    sysroot_resolve_test.cpp
    ${CMAKE_SOURCE_DIR}/src/exec/Sysroot.cpp)
target_include_directories(wvmcc-sysroot-resolve-test PRIVATE
    ${CMAKE_SOURCE_DIR}/src)
add_test(NAME sysroot-resolve COMMAND wvmcc-sysroot-resolve-test)
