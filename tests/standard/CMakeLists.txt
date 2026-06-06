# tests/standard — C17 standard-compliance test suite.
#
# Materializes the catalog in docs/standard/ (one row -> one assertion). This
# first batch wires the `static-assert` Verify mode: compile-only, in
# `-ffreestanding`, against wvmcc's own headers in runtime/include. These tests
# need neither the linker nor wasmvm — a test passes iff wvmcc compiles the TU
# (all `_Static_assert`s held) and fails (non-zero exit) otherwise.
#
# Later batches add the `exit` / `stdout` (run on wasmvm) and `compile-fail`
# Verify modes; `compile-fail` is blocked on the pp/parse exit-code fix (#80).
#
# NOTE: compile-time header-constant suites are bundled one file per header
# (e.g. libc/limits.c covers every LIBC-limits-* row); runtime function rows
# will be one file per entity per docs/standard/README.md.

set(RUNTIME_INCDIR ${CMAKE_SOURCE_DIR}/runtime/include)

# add_standard_static_assert(<name> <source>)
#   Registers a compile-only freestanding test. Passes iff wvmcc exits 0.
function(add_standard_static_assert name source)
    set(status supported)
    if(ARGC GREATER 2)
        set(status "${ARGV2}")
    endif()
    set(out ${CMAKE_CURRENT_BINARY_DIR}/${name}.wasm)
    add_test(
        NAME std-${name}
        COMMAND $<TARGET_FILE:wvmcc> -ffreestanding -isystem ${RUNTIME_INCDIR}
                ${source} -o ${out}
    )
    set_tests_properties(std-${name} PROPERTIES
        LABELS "standard;static-assert;status-${status}" TIMEOUT 30)
endfunction()

# --- §7.10 <limits.h> / §7.20 <stdint.h> / §7.18 <stdbool.h> ---
# These cover the integer-constant-MACRO rows (values, #if-usability).
#
# Scope limits of this first batch:
#  - Type-WIDTH / signedness / alignment rows (sizeof/_Alignof/casts in an ICE)
#    are blocked on #81 (wvmcc's _Static_assert evaluator rejects them). That
#    excludes all of <stddef.h>'s static-assert rows for now, plus the width
#    checks in <stdint.h>/<stdbool.h> — they land once #81 is fixed.
#  - §7.7 <float.h>, §7.9 <iso646.h>, §7.15 <stdalign.h>, §7.23 <stdnoreturn.h>
#    are freestanding-required (4p6) but not yet present in runtime/include;
#    their rows stay `partial` until the headers exist.
add_standard_static_assert(libc-limits  ${CMAKE_CURRENT_SOURCE_DIR}/libc/limits.c)
add_standard_static_assert(libc-stdint  ${CMAKE_CURRENT_SOURCE_DIR}/libc/stdint.c)
add_standard_static_assert(libc-stdbool ${CMAKE_CURRENT_SOURCE_DIR}/libc/stdbool.c)

# --- `exit` Verify mode: compile, link against libc.a, run on wasmvm ---
# A test passes iff the linked module runs and `main` returns 0 (the program
# returns non-zero on its first failed assertion). Mirrors the proven
# `add_link_test` from tests/integration. Skipped if wasmvm is not on PATH.
set(RUNTIME_LIBDIR ${CMAKE_BINARY_DIR}/runtime/lib)
find_program(WASMVM_EXECUTABLE NAMES wasmvm)
if(NOT WASMVM_EXECUTABLE)
    message(STATUS "wasmvm not found — skipping tests/standard `exit` rows")
endif()

# add_standard_run_test(<name> <source>)
function(add_standard_run_test name source)
    if(NOT WASMVM_EXECUTABLE)
        return()
    endif()
    set(status supported)
    if(ARGC GREATER 2)
        set(status "${ARGV2}")
    endif()
    set(out ${CMAKE_CURRENT_BINARY_DIR}/${name}.wasm)
    add_test(
        NAME std-${name}
        COMMAND ${CMAKE_COMMAND} -E env sh -c
            "$<TARGET_FILE:wvmcc> -isystem '${RUNTIME_INCDIR}' '${source}' -L '${RUNTIME_LIBDIR}' -lc -o '${out}' && '${WASMVM_EXECUTABLE}' '${out}'"
    )
    set_tests_properties(std-${name} PROPERTIES
        LABELS "standard;run;status-${status}" TIMEOUT 15)
endfunction()

# add_standard_run_test2(<name> <src1> <src2> [status]) — two-TU link+run (for
# cross-translation-unit linkage/compatibility rows).
function(add_standard_run_test2 name src1 src2)
    if(NOT WASMVM_EXECUTABLE)
        return()
    endif()
    set(status supported)
    if(ARGC GREATER 3)
        set(status "${ARGV3}")
    endif()
    set(out ${CMAKE_CURRENT_BINARY_DIR}/${name}.wasm)
    add_test(
        NAME std-${name}
        COMMAND ${CMAKE_COMMAND} -E env sh -c
            "$<TARGET_FILE:wvmcc> -isystem '${RUNTIME_INCDIR}' '${src1}' '${src2}' -L '${RUNTIME_LIBDIR}' -lc -o '${out}' && '${WASMVM_EXECUTABLE}' '${out}'"
    )
    set_tests_properties(std-${name} PROPERTIES
        LABELS "standard;run;status-${status}" TIMEOUT 15)
endfunction()

# §7.4 <ctype.h> — static-inline classification/mapping; no libc TU, so robust
# on WasmVM. (isblank is absent from runtime/include/ctype.h → deferred.)
foreach(fn isdigit isalpha isalnum isupper islower isxdigit isspace
           iscntrl isprint isgraph ispunct tolower toupper)
    add_standard_run_test(libc-ctype-${fn} ${CMAKE_CURRENT_SOURCE_DIR}/libc/ctype/${fn}.c)
endforeach()

# §7.24 <string.h> — minimal, robust slice only. The installed WasmVM
# interpreter traps on some of string.c's i64/size_t loops with larger retained
# call graphs (see tests/integration/phase5/e2e_libc_string.c), so this batch
# stays to short inputs / tiny call graphs; broader string rows follow once the
# interpreter issue is resolved.
add_standard_run_test(libc-string-strlen ${CMAKE_CURRENT_SOURCE_DIR}/libc/string/strlen.c)
add_standard_run_test(libc-string-strcmp ${CMAKE_CURRENT_SOURCE_DIR}/libc/string/strcmp.c)

# --- `stdout` Verify mode: run on wasmvm and diff captured stdout vs a fixture ---
# add_standard_stdout_test(<name> <source> <expected-file>)
function(add_standard_stdout_test name source expected)
    if(NOT WASMVM_EXECUTABLE)
        return()
    endif()
    set(status supported)
    if(ARGC GREATER 3)
        set(status "${ARGV3}")
    endif()
    set(out ${CMAKE_CURRENT_BINARY_DIR}/${name}.wasm)
    add_test(
        NAME std-${name}
        COMMAND ${CMAKE_COMMAND} -E env sh -c
            "$<TARGET_FILE:wvmcc> -isystem '${RUNTIME_INCDIR}' '${source}' -L '${RUNTIME_LIBDIR}' -lc -o '${out}' && sh '${CMAKE_CURRENT_SOURCE_DIR}/run_standard_stdout.sh' '${WASMVM_EXECUTABLE}' '${out}' '${expected}'"
    )
    set_tests_properties(std-${name} PROPERTIES
        LABELS "standard;stdout;status-${status}" TIMEOUT 15)
endfunction()

# §7.21 <stdio.h> — formatted/character output (LIBC-stdio-*). printf/puts are
# exercised by existing integration tests, so the call graph is wasmvm-safe.
set(STDIO_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libc/stdio)
add_standard_stdout_test(libc-stdio-puts          ${STDIO_DIR}/puts.c          ${STDIO_DIR}/puts.expected)
add_standard_stdout_test(libc-stdio-putchar       ${STDIO_DIR}/putchar.c       ${STDIO_DIR}/putchar.expected)
add_standard_stdout_test(libc-stdio-printf-int    ${STDIO_DIR}/printf_int.c    ${STDIO_DIR}/printf_int.expected)
add_standard_stdout_test(libc-stdio-printf-width  ${STDIO_DIR}/printf_width.c  ${STDIO_DIR}/printf_width.expected)
add_standard_stdout_test(libc-stdio-printf-flags  ${STDIO_DIR}/printf_flags.c  ${STDIO_DIR}/printf_flags.expected)
add_standard_stdout_test(libc-stdio-printf-string ${STDIO_DIR}/printf_string.c ${STDIO_DIR}/printf_string.expected)
add_standard_stdout_test(libc-stdio-printf-long   ${STDIO_DIR}/printf_long.c   ${STDIO_DIR}/printf_long.expected)

# --- `compile-fail` Verify mode (unblocked by #80) ---
# An ill-formed (constraint/syntax-violating) TU must be REJECTED. The test
# command is just `wvmcc`; WILL_FAIL inverts it, so the test passes iff wvmcc
# exits non-zero. Message text is not asserted (no stable diagnostic IDs).
#
# Only constraints wvmcc actually diagnoses today are listed here (each was
# empirically confirmed to be rejected). Constraints wvmcc does not yet detect
# (e.g. sizeof(void), assign-to-const, ptr+ptr, `.` on non-struct, tag-kind
# mismatch, too-many-args, redefinition of a block-scope object, bad bit-field
# width, non-positive array size) are missing-diagnostic gaps tracked separately
# and stay `partial` in the catalog.
function(add_standard_compile_fail name source)
    set(status supported)
    if(ARGC GREATER 2)
        set(status "${ARGV2}")
    endif()
    set(out ${CMAKE_CURRENT_BINARY_DIR}/${name}.wasm)
    add_test(
        NAME std-${name}
        COMMAND $<TARGET_FILE:wvmcc> -ffreestanding -isystem ${RUNTIME_INCDIR}
                ${source} -o ${out}
    )
    set_tests_properties(std-${name} PROPERTIES
        WILL_FAIL TRUE LABELS "standard;compile-fail;status-${status}" TIMEOUT 30)
endfunction()

# 6.4 / 6.5 expressions
add_standard_compile_fail(lang-6.4-stray-token        ${CMAKE_CURRENT_SOURCE_DIR}/language/6.4_lexical/stray_token.c)
add_standard_compile_fail(lang-6.5.1-undeclared       ${CMAKE_CURRENT_SOURCE_DIR}/language/6.5_expressions/undeclared_identifier.c)
add_standard_compile_fail(lang-6.5.2.1-subscript      ${CMAKE_CURRENT_SOURCE_DIR}/language/6.5_expressions/subscript_non_array.c)
add_standard_compile_fail(lang-6.5.3.2-deref          ${CMAKE_CURRENT_SOURCE_DIR}/language/6.5_expressions/deref_non_pointer.c)
# 6.7 declarations
add_standard_compile_fail(lang-6.7-empty-decl         ${CMAKE_CURRENT_SOURCE_DIR}/language/6.7_declarations/empty_declaration.c)
add_standard_compile_fail(lang-6.7-incompatible-redecl ${CMAKE_CURRENT_SOURCE_DIR}/language/6.7_declarations/incompatible_redecl.c)
add_standard_compile_fail(lang-6.7.2.2-enum-dup       ${CMAKE_CURRENT_SOURCE_DIR}/language/6.7_declarations/enum_duplicate.c)
add_standard_compile_fail(lang-6.7.2.3-redefine-struct ${CMAKE_CURRENT_SOURCE_DIR}/language/6.7_declarations/redefine_struct.c)
add_standard_compile_fail(lang-6.7.9-excess-init      ${CMAKE_CURRENT_SOURCE_DIR}/language/6.7_declarations/excess_initializer.c)
add_standard_compile_fail(lang-6.7.10-sassert-zero    ${CMAKE_CURRENT_SOURCE_DIR}/language/6.7_declarations/static_assert_zero.c)
add_standard_compile_fail(lang-6.7.10-sassert-nonconst ${CMAKE_CURRENT_SOURCE_DIR}/language/6.7_declarations/static_assert_nonconst.c)
# 6.8 statements
add_standard_compile_fail(lang-6.8.1-case-outside     ${CMAKE_CURRENT_SOURCE_DIR}/language/6.8_statements/case_outside_switch.c)
add_standard_compile_fail(lang-6.8.4.2-dup-case       ${CMAKE_CURRENT_SOURCE_DIR}/language/6.8_statements/dup_case_label.c)
add_standard_compile_fail(lang-6.8.6.1-goto-undef     ${CMAKE_CURRENT_SOURCE_DIR}/language/6.8_statements/goto_undefined_label.c)
add_standard_compile_fail(lang-6.8.6.2-continue       ${CMAKE_CURRENT_SOURCE_DIR}/language/6.8_statements/continue_outside_loop.c)
add_standard_compile_fail(lang-6.8.6.3-break          ${CMAKE_CURRENT_SOURCE_DIR}/language/6.8_statements/break_outside.c)
add_standard_compile_fail(lang-6.8.6.4-return-void    ${CMAKE_CURRENT_SOURCE_DIR}/language/6.8_statements/return_value_in_void.c)
add_standard_compile_fail(lang-6.8.6.4-bare-return    ${CMAKE_CURRENT_SOURCE_DIR}/language/6.8_statements/bare_return_nonvoid.c)
# 6.9 external definitions
add_standard_compile_fail(lang-6.9.1-redefine-func    ${CMAKE_CURRENT_SOURCE_DIR}/language/6.9_external/redefine_function.c)

# --- Generated scaffold (full catalog materialization) ---
# Each tests/standard/scaffold/<clause>.cmake registers the standard-conformance
# tests for one clause/header group via the helpers above, passing the catalog
# Status as the label arg. Tests assert the STANDARD; partial/deferred areas are
# expected to FAIL (a conformance signal), filterable by `status-*` labels.
file(GLOB _std_scaffold ${CMAKE_CURRENT_SOURCE_DIR}/scaffold/*.cmake)
foreach(_sc ${_std_scaffold})
    include(${_sc})
endforeach()
