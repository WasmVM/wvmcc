add_executable(wvmcc-preprocess-only-test preprocess_only_test.cpp)
add_dependencies(wvmcc-preprocess-only-test wvmcc)
add_test(NAME preprocess-only COMMAND wvmcc-preprocess-only-test)

add_executable(wvmcc-sysroot-resolve-test
    sysroot_resolve_test.cpp
    ${CMAKE_SOURCE_DIR}/src/exec/Sysroot.cpp)
target_include_directories(wvmcc-sysroot-resolve-test PRIVATE
    ${CMAKE_SOURCE_DIR}/src)
add_test(NAME sysroot-resolve COMMAND wvmcc-sysroot-resolve-test)

add_executable(wvmcc-cli-flags-test cli_flags_test.cpp)
add_dependencies(wvmcc-cli-flags-test wvmcc)
add_test(NAME cli-flags COMMAND wvmcc-cli-flags-test)

# #80: preprocessor/parser errors force a non-zero exit (warnings do not).
add_executable(wvmcc-diagnostic-exit-status-test diagnostic_exit_status_test.cpp)
add_dependencies(wvmcc-diagnostic-exit-status-test wvmcc)
add_test(NAME diagnostic-exit-status COMMAND wvmcc-diagnostic-exit-status-test)
