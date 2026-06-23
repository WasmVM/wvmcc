#!/usr/bin/env sh
# run_standard_stderr.sh <wasmvm> <module.wasm> <expected-file>
# Like run_standard_stdout.sh, but captures the program's *combined* output
# (stdout + stderr) — for diagnostics the standard sends to stderr, e.g. a
# failed `assert`. The program is expected to terminate abnormally (abort), so
# its non-zero exit status is ignored; the captured text is the contract.
set -eu
wasmvm_bin=$1
wasm=$2
expected=$3

actual=$(mktemp)
trap 'rm -f "$actual"' EXIT

"$wasmvm_bin" "$wasm" >"$actual" 2>&1 || true

diff -u "$expected" "$actual"
