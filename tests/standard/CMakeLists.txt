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
    set(out ${CMAKE_CURRENT_BINARY_DIR}/${name}.wasm)
    add_test(
        NAME std-${name}
        COMMAND $<TARGET_FILE:wvmcc> -ffreestanding -isystem ${RUNTIME_INCDIR}
                ${source} -o ${out}
    )
    set_tests_properties(std-${name} PROPERTIES LABELS "standard;static-assert" TIMEOUT 30)
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
    set(out ${CMAKE_CURRENT_BINARY_DIR}/${name}.wasm)
    add_test(
        NAME std-${name}
        COMMAND ${CMAKE_COMMAND} -E env sh -c
            "$<TARGET_FILE:wvmcc> -isystem '${RUNTIME_INCDIR}' '${source}' -L '${RUNTIME_LIBDIR}' -lc -o '${out}' && '${WASMVM_EXECUTABLE}' '${out}'"
    )
    set_tests_properties(std-${name} PROPERTIES LABELS "standard;run" TIMEOUT 15)
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
