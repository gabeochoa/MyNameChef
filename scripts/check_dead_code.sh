#!/bin/bash

# Dead Code Detection Script
# Uses clang-tidy to find unused code

echo "🔍 Checking for dead code with clang-tidy..."

# Check if clang-tidy is available
if ! command -v clang-tidy &> /dev/null; then
    echo "❌ clang-tidy not found. Install with: brew install llvm"
    exit 1
fi

# Run clang-tidy with dead code checks
# -readability-avoid-unused-parameters: unused function parameters
# -misc-unused-using-decls: unused using declarations  
# -misc-unused-alias-decls: unused type aliases
# -readability-unused-member-function: unused member functions
# Note: We exclude vendor/ to avoid noise

clang-tidy \
    src/**/*.cpp src/**/*.h \
    -- \
    -std=c++23 \
    -Ivendor/afterhours \
    -Ivendor \
    -Wno-unused-function \
    -checks=readability-avoid-unused-parameters,\
misc-unused-using-decls,\
misc-unused-alias-decls,\
readability-unused-member-function,\
misc-unused-parameters \
    2>&1 | grep -E "(warning:|error:)" | head -100

echo ""
echo "✅ Dead code check complete"
echo ""
echo "💡 Tip: To find unused systems, check src/main.cpp for system registrations"
echo "💡 Tip: Run 'make cppcheck' for additional static analysis"

