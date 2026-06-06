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
