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

# add_link_test(<name> <source.c>)
#   M2-L4: compiles <source.c> through the *integrated linker*, lazily pulling
#   the libc TUs it needs from runtime/lib/libc.a, then runs the linked binary
#   under wasmvm. Exercises the full link pipeline (merge → crt0 → resolve →
#   dead-code → lazy archive pull) end-to-end. Requires the `runtime` target's
#   libc.a, which the default build produces.
set(RUNTIME_LIBDIR ${CMAKE_BINARY_DIR}/runtime/lib)
set(RUNTIME_INCDIR ${CMAKE_SOURCE_DIR}/runtime/include)
function(add_link_test name source)
    set(wasm_out ${INTEGRATION_TEST_DIR}/${name}.wasm)
    add_test(
        NAME e2e-${name}
        COMMAND ${CMAKE_COMMAND} -E env
            sh -c "$<TARGET_FILE:wvmcc> -isystem '${RUNTIME_INCDIR}' '${source}' -L '${RUNTIME_LIBDIR}' -lc -o '${wasm_out}' && '${WASMVM_EXECUTABLE}' '${wasm_out}'"
    )
    set_tests_properties(e2e-${name} PROPERTIES TIMEOUT 15)
endfunction()

# add_link_stdout_test(<name> <source.c> <expected-stdout>)
#   Like add_link_test, but asserts the program's captured stdout exactly equals
#   <expected-stdout> (in addition to a zero exit). Used to verify output-side
#   behavior such as flush-at-exit, where the exit code alone says nothing.
function(add_link_stdout_test name source expected)
    set(wasm_out ${INTEGRATION_TEST_DIR}/${name}.wasm)
    add_test(
        NAME e2e-${name}
        COMMAND ${CMAKE_COMMAND} -E env
            sh -c "$<TARGET_FILE:wvmcc> -isystem '${RUNTIME_INCDIR}' '${source}' -L '${RUNTIME_LIBDIR}' -lc -o '${wasm_out}' && out=$('${WASMVM_EXECUTABLE}' '${wasm_out}') && [ \"$out\" = '${expected}' ]"
    )
    set_tests_properties(e2e-${name} PROPERTIES TIMEOUT 15)
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

# M2-L4 — lazy archive pulls: link a user TU against libc.a end-to-end.
add_link_test(libc-string            ${CMAKE_CURRENT_SOURCE_DIR}/phase5/e2e_libc_string.c)

# stdio flush-at-exit: line-buffered stdout with no trailing newline must still
# appear on normal termination (crt0 calls libc's __stdio_exit before exit).
add_link_stdout_test(stdio-flush     ${CMAKE_CURRENT_SOURCE_DIR}/phase5/e2e_stdio_flush.c "Hello")

# _Noreturn: a function ending in a no-return call (exit/abort/user _Noreturn)
# must emit a trailing `unreachable` so a non-void caller stays valid.
add_link_test(noreturn               ${CMAKE_CURRENT_SOURCE_DIR}/phase5/e2e_noreturn.c)

# Flush on explicit exit(): buffered output must survive a call to exit() that
# never returns to crt0 (libc exit() calls __stdio_exit before terminating).
add_link_stdout_test(exit-flush      ${CMAKE_CURRENT_SOURCE_DIR}/phase5/e2e_exit_flush.c "before-exit")

# M2-14 — float printf via hand-rolled %f / %e / %g / %a. Returns 0 iff every
# acceptance-criterion conversion produces the expected output (compared via
# snprintf + strcmp inside the program).
add_link_test(printf-float           ${CMAKE_CURRENT_SOURCE_DIR}/phase5/e2e_printf_float.c)

# M2-14 — NaN / +Inf / -Inf printing via <math.h>. Stdout-fixture check.
add_link_stdout_test(printf-float-special
                                     ${CMAKE_CURRENT_SOURCE_DIR}/phase5/e2e_printf_float_special.c
                                     "nan inf -inf")
