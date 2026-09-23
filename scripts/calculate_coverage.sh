#!/bin/bash
# Issues 81/95: per-process profiles, propagate failures, verify report exists.
set -e
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BASE_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
cd "$BASE_DIR"
export LLVM_PROFILE_FILE="$BASE_DIR/coverage/prof/%p.profraw"
mkdir -p "$BASE_DIR/coverage/prof"
echo "Building with coverage..."
make COVERAGE=1
echo "Running tests..."
./scripts/run_tests.py --skip-lint
echo "Collecting coverage data..."
COVERAGE_DIR="$BASE_DIR/coverage"
mkdir -p "$COVERAGE_DIR"
if [[ "$OSTYPE" == "darwin"* ]]; then
    PROFRAW_FILES=$(find "$COVERAGE_DIR/prof" . -name "*.profraw" 2>/dev/null | head -n 200)
    if [ -z "$PROFRAW_FILES" ]; then echo "No .profraw files found" >&2; exit 1; fi
    xcrun llvm-profdata merge -sparse $PROFRAW_FILES -o "$COVERAGE_DIR/default.profdata"
    xcrun llvm-cov show ./output/my_name_chef.exe -instr-profile="$COVERAGE_DIR/default.profdata" -format=html -output-dir="$COVERAGE_DIR/html" -ignore-filename-regex="vendor|test"
else
    lcov --capture --directory . --output-file "$COVERAGE_DIR/coverage.info" --exclude "*/vendor/*" --exclude "*/test/*"
    genhtml "$COVERAGE_DIR/coverage.info" --output-directory "$COVERAGE_DIR/html"
fi
test -f "$COVERAGE_DIR/html/index.html" || { echo "Coverage report not generated" >&2; exit 1; }
echo "Coverage report generated at: $COVERAGE_DIR/html/index.html"
