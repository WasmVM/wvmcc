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

# #28: enhanced diagnostics — file:line:col header, source snippet + caret, hint.
add_executable(wvmcc-diagnostic-caret-test diagnostic_caret_test.cpp)
add_dependencies(wvmcc-diagnostic-caret-test wvmcc)
add_test(NAME diagnostic-caret COMMAND wvmcc-diagnostic-caret-test)

# #27: comprehensive error handling — call-site type mismatch is a named
# located error, no cascading diagnostics, per-function recovery, and valid
# implicit conversions never rejected.
add_executable(wvmcc-error-handling-test error_handling_test.cpp)
add_dependencies(wvmcc-error-handling-test wvmcc)
add_test(NAME error-handling COMMAND wvmcc-error-handling-test)
