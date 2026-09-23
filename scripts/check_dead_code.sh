#!/bin/bash
# Issue 92: clang-tidy options belong before `--`; propagate exit status.
set -o pipefail
echo "Checking for dead code with clang-tidy..."
if ! command -v clang-tidy &> /dev/null; then
    echo "clang-tidy not found. Install with: brew install llvm" >&2
    exit 1
fi
clang-tidy -checks='readability-avoid-unused-parameters,misc-unused-using-decls,misc-unused-alias-decls,readability-unused-member-function,misc-unused-parameters' \
    src/*.cpp src/components/*.cpp src/systems/*.cpp -- \
    -std=c++23 -Isrc -Ivendor/afterhours -Ivendor -Wno-unused-function \
    2>&1 | grep -E "(warning:|error:)" | head -100
status=${PIPESTATUS[0]}
if [ $status -ne 0 ]; then echo "clang-tidy failed ($status)" >&2; exit $status; fi
echo "Dead code check complete"
