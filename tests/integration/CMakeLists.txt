# Issue #41: end-to-end integration tests.
#
# Each test compiles a small C program with wvmcc and runs the resulting wasm
# under wasmvm. The C program returns 0 on success and a non-zero exit code on
# the first failed assertion, which ctest reports as a failure.
#
# wasmvm is an external dependency; if it isn't on PATH the integration tests
# are skipped at configure time.

find_program(WASMVM_EXECUTABLE NAMES wasmvm)

if(NOT WASMVM_EXECUTABLE)
    message(STATUS "wasmvm not found on PATH — skipping integration tests")
    return()
endif()

message(STATUS "Integration tests will use wasmvm: ${WASMVM_EXECUTABLE}")

set(INTEGRATION_TEST_DIR ${CMAKE_CURRENT_BINARY_DIR})

# add_e2e_test(<name> <source.c>)
#   Compiles <source.c> with wvmcc and runs the resulting wasm under wasmvm.
#   Test passes iff wasmvm exits 0 (i.e., the C `main` returned 0). A 10-second
#   ctest timeout guards against codegen bugs that produce infinite loops.
#
#   wasmvm CLI (post-WasmVM 5966c7d): host args are positional after the
#   module path — `wasmvm out.wasm foo bar` (was `--args foo bar`). Module
#   search paths moved to `--extra/-e`. Tests here pass no args, so the
#   invocation is the same under both old and new CLIs.
#   M2-F: every e2e test passes `-ffreestanding` so it gets a self-contained
#   module (own memories, own stack pointer, start-wrapper around main). Once
#   M2-D flips the default to linkable-object mode, the flag becomes load-
#   bearing — until the linker (M2-L1..L10) lands, only `-ffreestanding`
#   modules can be run standalone under wasmvm.
function(add_e2e_test name source)
    set(wasm_out ${INTEGRATION_TEST_DIR}/${name}.wasm)
    add_test(
        NAME e2e-${name}
        COMMAND ${CMAKE_COMMAND} -E env
            sh -c "$<TARGET_FILE:wvmcc> -ffreestanding '${source}' -o '${wasm_out}' && '${WASMVM_EXECUTABLE}' '${wasm_out}'"
    )
    set_tests_properties(e2e-${name} PROPERTIES TIMEOUT 10)
endfunction()

# Phase 2 — memory and aggregates
add_e2e_test(phase2-struct           ${CMAKE_CURRENT_SOURCE_DIR}/phase2/e2e_struct.c)
add_e2e_test(phase2-pointer-arith    ${CMAKE_CURRENT_SOURCE_DIR}/phase2/e2e_pointer_arith.c)
add_e2e_test(phase2-string-literal   ${CMAKE_CURRENT_SOURCE_DIR}/phase2/e2e_string_literal.c)

# Phase 3 — control flow
add_e2e_test(phase3-loops            ${CMAKE_CURRENT_SOURCE_DIR}/phase3/e2e_loops.c)
add_e2e_test(phase3-switch           ${CMAKE_CURRENT_SOURCE_DIR}/phase3/e2e_switch.c)
add_e2e_test(phase3-goto             ${CMAKE_CURRENT_SOURCE_DIR}/phase3/e2e_goto.c)

# Phase 4 — advanced features
add_e2e_test(phase4-func-pointer     ${CMAKE_CURRENT_SOURCE_DIR}/phase4/e2e_func_pointer.c)
add_e2e_test(phase4-static-local     ${CMAKE_CURRENT_SOURCE_DIR}/phase4/e2e_static_local.c)

# M2-A — variadic function support (compiler prereq for M2 libc/printf)
add_e2e_test(phase5-variadic         ${CMAKE_CURRENT_SOURCE_DIR}/phase5/e2e_variadic.c)

# M2-B — float arithmetic codegen
add_e2e_test(phase5-float            ${CMAKE_CURRENT_SOURCE_DIR}/phase5/e2e_float.c)
add_e2e_test(phase5-variadic-float   ${CMAKE_CURRENT_SOURCE_DIR}/phase5/e2e_variadic_float.c)

# #77 — file-scope (global) variable codegen: scalar/array/struct globals,
# initializers (data segments), zero-init, and cross-function mutation.
add_e2e_test(phase5-globals          ${CMAKE_CURRENT_SOURCE_DIR}/phase5/e2e_globals.c)

# M2-13 prereq — `continue` in a for-loop must run the step (regression).
add_e2e_test(phase5-for-continue     ${CMAKE_CURRENT_SOURCE_DIR}/phase5/e2e_for_continue.c)
