#!/usr/bin/env sh
# run_standard_stdout.sh <wasmvm> <module.wasm> <expected-file>
# Runs the module on wasmvm, captures stdout, and diffs it (exact bytes,
# including the trailing newline) against the expected fixture. Exits non-zero
# on any mismatch, which ctest reports as a failure.
set -eu
wasmvm_bin=$1
wasm=$2
expected=$3

actual=$(mktemp)
trap 'rm -f "$actual"' EXIT

# Capture stdout; ignore the program's exit status here (stdout is the contract).
"$wasmvm_bin" "$wasm" >"$actual" 2>/dev/null || true

diff -u "$expected" "$actual"
