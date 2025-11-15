#!/bin/bash

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BASE_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
cd "$BASE_DIR"

echo "Building with coverage..."
xmake config --coverage=y
xmake

echo ""
echo "Running tests..."
./scripts/run_tests.py --skip-lint

echo ""
echo "Collecting coverage data..."

COVERAGE_DIR="$BASE_DIR/coverage"
mkdir -p "$COVERAGE_DIR"

if [[ "$OSTYPE" == "darwin"* ]]; then
    # macOS - use llvm-cov
    echo "Generating coverage report (macOS/llvm-cov)..."
    
    # Find profraw files
    PROFRAW_FILES=$(find . -name "*.profraw" 2>/dev/null || true)
    
    if [ -z "$PROFRAW_FILES" ]; then
        echo "Warning: No .profraw files found"
        exit 1
    fi
    
    # Merge profraw files
    xcrun llvm-profdata merge -sparse *.profraw -o "$COVERAGE_DIR/default.profdata" 2>/dev/null || true
    
    # Generate HTML report
    xcrun llvm-cov show ./output/my_name_chef.exe \
        -instr-profile="$COVERAGE_DIR/default.profdata" \
        -format=html \
        -output-dir="$COVERAGE_DIR/html" \
        -ignore-filename-regex="vendor|test" || true
    
    echo "Coverage report generated at: $COVERAGE_DIR/html/index.html"
else
    # Linux/Windows - use gcov/lcov
    echo "Generating coverage report (gcov/lcov)..."
    
    # Generate lcov report
    lcov --capture --directory . --output-file "$COVERAGE_DIR/coverage.info" \
        --exclude "*/vendor/*" \
        --exclude "*/test/*" || true
    
    # Generate HTML report
    genhtml "$COVERAGE_DIR/coverage.info" --output-directory "$COVERAGE_DIR/html" || true
    
    echo "Coverage report generated at: $COVERAGE_DIR/html/index.html"
fi

echo ""
echo "Coverage calculation complete!"

